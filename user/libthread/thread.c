/*
 *  @file thread.c
 *
 *  @brief Implementation of thread library
 *
 *  We keep the thread id assigned by kernel as the tid to library user, so that
 *  thr_getid and thr_yield can be directly mapped to corresponding syscalls
 *
 *  Each thread has its own TCB (Thread Control Block) stored here, which
 *  records the status of thread. All access (except dying thread) to TCBs must
 *  be protected under the mutex.
 *
 *  The most important field in TCB is state, which reveals the lifeline of
 *  a thread:
 *  BIRTH: assigned by the parent thread, in this state the TCB has valid ID,
 *         allocated stack block, and the target thread is in the phase of
 *         bootstrapping
 *  ALIVE: assigned by target thread, transited only from BIRTH. Now the target
 *         thread is executing its function and not completed (neither thr_exit
 *         nor the payload function returns)
 *  [DYING]: dying is complex since it consists of three substate, as long as
 *         DEAD bit is set, the state is consiered DYING:
 *         - DEAD: the payload function has completed, and return state has been
 *                 recorded. This is set when target thread is reaching
 *                 thr_exit();
 *         - ROTTEN: set by target thread, when he knows that he'll never access
 *                 its own stack block again. After this flag is set, the stack
 *                 of target thread is susceptible to be revoked at any time
 *         - JOINT: set by parent thread, when calling thr_join; this will take
 *                 away return state
 *         NOTE: setting ROTTEN cannot be protected by mutex because unlock
 *         is using stack. So all transition in [DYING] must be atmoic.
 *  VACANT: when the state is having both ROTTEN and JOINT, it is ready to be
 *         reclaimed. note that the TCB may still holding a memory block, and
 *         the reclaimer should deallocate it (reclaimer is the next thread who
 *         tries to create thread, this is lazy reclaim)
 *
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <simics.h>
#include <syscall.h>
#include <stddef.h>
#include <mutex.h>
#include <string.h>
#include <cond.h>
#include <thread.h>
#include <autostack_thread.h>

#include "thread_create.h"
#include "malloc_internal.h"

typedef enum ThreadState {
  BIRTH = 0,
  ALIVE = 1,

  DEAD = 8,
  ROTTEN = 16,
  JOINT = 32,

  VACANT = 16 | 32
} ThreadState;

#define IS_VACANT(x) (((x) & VACANT) == VACANT)
#define IS_DEAD(x) (((x) & DEAD) == DEAD)

// We actually implement a mini-condvar (only one subscriber) in TCB, in the
// same method, with members waiterTID and waiterAwaken.
// We don't use condvar because it's too heavy: we only need one subscriber
typedef struct TCB {
  threadStackBlock* memBlock;
  int ID;

  ThreadState state;
  void* ret;     // The returned state

  int waiterTID;  // If >=0, some thread with this ID is waiting
  int* waiterAwaken;  // The atomic flag to shortcut-prevent
} TCB;

static mutex_t threadStatesProtector;
static TCB threads[THREAD_NUM_LIMIT];

// Non-thread safe, please protect under threadStatesProtector
// If TID >= 0, Find a valid TCB with ID = TID
// If TID < 0, Find the 1st vacant TCB
// NULL for find failure
static TCB* findTCB(int TID) {
  for (int i = 0; i < THREAD_NUM_LIMIT; i++) {
    if (TID >= 0 && !IS_VACANT(threads[i].state) && threads[i].ID == TID) {
      return &threads[i];
    }
    if (TID < 0 && IS_VACANT(threads[i].state)) {
      return &threads[i];
    }
  }
  return NULL;
}

/******************************************************************************/

// This function is used for starting ThreadPayload. It will never return,
// instead, it end up with vanish();
void newThreadEntry(ThreadPayload func, void *args) {
  // Sync with parent thread and set the state to running
  int tid = gettid();
  mutex_lock(&threadStatesProtector);
  TCB* tcb = findTCB(tid);
  // Note that all multithreaded handlers runs on the bottom of own stack,
  // since the handler will panic directly, there's no need to preserve
  // current stack anymore
  installMultithreadHandler(tcb->memBlock->high - 1);
  tcb->state = ALIVE;
  mutex_unlock(&threadStatesProtector);
  thr_exit(func(args));
}

int thr_init(unsigned int size) {
  size = ALIGN_PAGE_CEIL(size);

  // Switch the autostack to multithreaded mode, and store the stack block for
  // this thread
  threadStackBlock* currentThreadBlock;
  int rc = switchToMultiThreadMode(size, &currentThreadBlock);
  if (rc < 0) {
    lprintf("Problems in swtich autostack to multi thread mode. rc = %d", rc);
    return rc;
  }

  // Switch *alloc family to multithreaded mode
  initMultithread();

  mutex_init(&threadStatesProtector);
  for (int i = 0; i < THREAD_NUM_LIMIT; i++) {
    threads[i].state = VACANT;
    threads[i].memBlock = NULL;
  }
  threads[0].memBlock = currentThreadBlock;
  threads[0].ID = gettid();
  threads[0].state = ALIVE;
  threads[0].waiterTID = -1;
  threads[0].waiterAwaken = NULL;
  lprintf("Thread library inited with stacksize = 0x%08x", size);
  return 0;
}

int thr_create(ThreadPayload func, void *args) {
  mutex_lock(&threadStatesProtector);

  // First, try find a spare TCB
  TCB* tcb = findTCB(-1);
  if (tcb == NULL) {
    mutex_unlock(&threadStatesProtector);
    lprintf("No spare TCB!");
    return -1;
  }

  // Deallocate the old memblock of the vacant one, if any
  if (tcb->memBlock) {
    deallocateThreadBlock(tcb->memBlock);
    tcb->memBlock = NULL;
  }

  // Then, allocate the thread stack block
  threadStackBlock* stackBlock = allocateThreadBlock();
  if (stackBlock == NULL) {
    mutex_unlock(&threadStatesProtector);
    return -1;
  }

  // We can create the thread now! New thread will be in newThreadEntry,
  // waiting for threadStatesProtector
  int tid = thread_create(stackBlock->high, newThreadEntry, func, args);
  if (tid < 0) {
    deallocateThreadBlock(stackBlock);
    mutex_unlock(&threadStatesProtector);
    lprintf("Fail to use thread_create to create threads. rc = %d", tid);
    return tid;
  }

  // Set the TCB
  tcb->memBlock = stackBlock;
  tcb->ID = tid;
  tcb->state = BIRTH;
  tcb->waiterTID = -1;
  tcb->waiterAwaken = NULL;
  mutex_unlock(&threadStatesProtector);
  return tid;
}

int thr_join(int tid, void **statusp) {
  mutex_lock(&threadStatesProtector);
  TCB* tcb = findTCB(tid);
  if (tcb == NULL) {
    // No thread found
    mutex_unlock(&threadStatesProtector);
    return -1;
  }
  if (tcb->waiterTID >= 0) {
    // this thread is being joint by the others
    mutex_unlock(&threadStatesProtector);
    return -1;
  }
  // Hey! now the while loop is identical to cond_wait implementation!
  int awakened = 0;
  tcb->waiterTID = gettid();
  tcb->waiterAwaken = &awakened;
  while (true) {
    awakened = 0;
    __sync_synchronize();
    if (IS_DEAD(tcb->state)) break;
    mutex_unlock(&threadStatesProtector);
    deschedule(&awakened);
    mutex_lock(&threadStatesProtector);
  }
  // Now tid is dead
  if (statusp) *statusp = tcb->ret;
  tcb->ret = NULL;
  tcb->waiterAwaken = NULL;
  __sync_fetch_and_or(&tcb->state, JOINT);
  mutex_unlock(&threadStatesProtector);

  return 0;
}

void thr_exit(void *status) {
  int tid = gettid();
  mutex_lock(&threadStatesProtector);
  TCB* tcb = findTCB(tid);
  if (tcb == NULL) {
    panic("Where is my TCB!? I'm thread #%d", tid);
  }
  // Claim into dead mode
  tcb->state = DEAD;
  tcb->ret = status;
  // It's identical to cond_signal
  if (tcb->waiterTID >= 0) {
    // wake up the waiter
    __sync_synchronize();
    *(tcb->waiterAwaken) = 1;
    make_runnable(tcb->waiterTID);
  }
  mutex_unlock(&threadStatesProtector);

  // Force all memory access on stack to be complete, so that this stack can
  // be reclaimed. So we use a memory barrier
  __sync_synchronize();
  // Set state to ROTTEN, we cannot touch my stack fron now on!!!
  __sync_fetch_and_or(&tcb->state, ROTTEN);

  // Good bye my thread!
  vanish();
}

int thr_getid() {
  return gettid();
}

int thr_yield(int tid) {
  return yield(tid);
}
