/** @file process.h
 *
 *  @brief Core data structure for PCB and TCB. Critical component
 *
 *  The description of TCB and PCB are detailed below; Besides the definition,
 *  this module only manages the internal storage of TCB/PCB itself. To specify,
 *  it's add/delete/find, and a global lock is used to only protect its present
 *  data sturcture.
 *
 *  XLX (eXpress LinX) is maintained to provide fast round-robin service. This
 *  module is oblivious to what should be kept in XLX. Instead, the caller does
 *  it, call corresponding api when changing thread status. Generally, threads
 *  in status (INIT, RUNNABLE, RUNNING, DEAD) are kept in XLX. And the rest
 *  (mostly BLOCKED/BLOCKED_USER) is out of XLX.
 *
 *  Besides those, this module is NOT RESPONSIBLE for any synchronization and
 *  race-prevention between different user of tcb/pcb.
 *
 *  TCB and PCB are kept in two linked list.
 *  Besides common GET to TCB/PCB, ephemeral access GET is exposed. When the
 *  caller is not owning it, and knows the threads/process may terminate anytime
 *  (which is the case of one thread accessing another, context switch, etc.),
 *  ephemeral access is used to prevent TCB/PCB data structure from being freed.
 *
 *  @author Leiyu Zhao
 */

#ifndef PROCESS_H
#define PROCESS_H

#include "sysconf.h"
#include "vm.h"
#include "loader.h"
#include "ureg.h"
#include "cpu.h"
#include "kmutex.h"
#include "hv.h"

/*************************Process Control Blok (PCB)****************************
 * PCB control all the states of one process (task in Pebble kernel)
 * ## The life of a process
 * - PROCESS_INITIALIZED: a normal process, when it's created, it's in this
 *   states. Note that there's no UNINITIALIZED for process
 * - PROCESS_PREZOMBIE: a pre-zombie process. It happens after the process is
 *   added to the zombie chain for its parent but before the last thread is
 *   reaped. In this state, CPU may still be in the address space of this
 *   process, so the parent process need to wait for it before clear
 * - PROCESS_ZOMBIE: when a prezombie's last thread is descheduled and reaped
 *   by reaper, the process is a *real* zombie. In this states, there's no
 *   chance to get to this process's mem space anymore, and the state is reduced
 *   to the return code.
 * - PROCESS_DEAD: waiter has caught the zombie. The process is dead. The reason
 *   it's still in memory is that someone has still kept ephemeral access of it.
 *
 ******************************************************************************/

typedef enum ProcessStatus {
  PROCESS_INITIALIZED = 1,
  PROCESS_PREZOMBIE = 2,
  PROCESS_ZOMBIE = 3,
  PROCESS_DEAD = 4
} ProcessStatus;

typedef struct _tcb tcb;

typedef struct _pcb {
  /* BEGIN: main chain cares */
  // id of the process
  int id;
  // next pcb in the main chain. Shouldn't be used by outsider
  struct _pcb* next;
  /* END: main chain cares */

  /* BEGIN: Read only after creation */
  // some user space memory layout
  ProcessMemoryMeta memMeta;
  // the page directory for this process
  PageDirectory pd;
  // the process's parent id. Will not be changed even if parent is dead
  int parentPID;
  // the first thread for the process. Only serves for fork() return
  int firstTID;
  // TODO
  HyperInfo hyperInfo;
  /* END: Read only after creation */

  /* BEGIN: Section C */

  // used to protect member access in Section C
  // Also, PageDirectory minor changes (new_pages) should be serial
  // However, initializing phase, fork and exec don't need its protection, since
  // they can only be called when the process only have one thread
  kmutex mutex;
  // the num of alive threads of the process. Note that dead (but not reaped)
  // threads don't count
  int numThread;
  // return code
  int retStatus;
  // used for providing -1 for wait(). When the number of wait() threads >=
  // total unwaited process
  int unwaitedChildProc;

  // Different meaning in diff cases:
  // 1. When states = ZOMBIE, it points to the next zombie sibling.
  // 2. Otherwise, it points to the first zombie child
  struct _pcb* zombieChain;
  // All blocked waiter threads of this process
  waiterLinklist* waiter;

  /* END: Section C */

  /* BEGIN: Section D */

  // only useful when status=prezombie. The waiter threads for this prezombie
  // to become a zombie. Must be a thread of my parent process
  void* prezombieWatcher; // tcb*

  // The spinlock used for accessing prezombieWatcher
  CrossCPULock prezombieWatcherLock;

  /* END: Section D */

  /* BEGIN: Section E */
  // See description above for all states and their transition.
  // Access to it may not be protected by any kmutex / spinlock. The accesser
  // should come up with some atomic operation
  ProcessStatus status;

  // Use to protect page table access, including accessing user space memory,
  // because kernel never knows when another thread of the process remove the
  // mem mapping without locking it.
  // It's used as a read-write lock, with all memory access as reading, and page
  // directoy modification as writing (actually, R/W on page table)
  kmutex memlock;

  /* END: Section E */


  /* BEGIN: Ephemeral access cares */
  // After this line all members should never be used by modules other than
  // process.c
  bool _hasAbandoned;
  int _ephemeralRefCount;
  /* END: Ephemeral access cares */
} pcb;


typedef struct _dualLinklist {
  struct _dualLinklist* prev;
  struct _dualLinklist* next;
  void* data;
} dualLinklist;


/*************************Thread Control Blok (TCB)****************************
 * TCB control all the states of one thread (the minimum schedule unit)
 * ## The life of a thread
 * - THREAD_UNINITIALIZED: the thread is just created, but a lot is not inited.
 *   zeus is still handling it and transit to THREAD_INITIALIZED
 * - THREAD_INITIALIZED: an initialized thread. It differs from RUNNABLE only
 *   in that it is never scheduled. It's a runnable thread and some one (either
 *   scheduler or the parent process) will context switch to it.
 * - THREAD_RUNNABLE: a runnable thread, and can be scheduled and context-switch
 *   to at any time
 * - THREAD_BLOCKED: this thread is somehow blocked. (waiting for kmutex, sleep
 *   waiting for keyboard, waiting for zombie children, etc.) Note that user-
 *   voluntary deschedule will lead to THREAD_BLOCKED_USER. The unblocker is
 *   responsible for transit the state to RUNNABLE.
 * - THREAD_RUNNING: running thread that occupies current CPU
 * - THREAD_DEAD: the thread is dead, happens when the thread kills itself by
 *   calling vanish(). When it transits, it has already deduce the numThread
 *   from its process (and may lead to prezombie change)
 * - THREAD_REAPED: reaper reaped the thread. and the thread is essentially
 *   non-exist. The only reason tcb is here is that someone is still having
 *   ephemeral access.
 * - THREAD_BLOCKED_USER: in the state if and only if the thread calls
 *   deschedule
 *
 * ## Thread ownership
 * A thread may be owned by someone, in several cases. And most of fields in TCB
 * can only be accessed when you own it. Scheduler can context switch to target
 * thread only if target thread is owned by scheduler.
 * - CPU will own the current running thread
 * - Scheduler will own the target thread before context switch to it. After
 *   that, scheduler disown previous thread and chown the new running thread to
 *   CPU.
 * - Unblocker will own the unblocking thread before changing thread status
 *
 ******************************************************************************/

typedef enum ThreadStatus {
  THREAD_UNINITIALIZED = 0,
  THREAD_INITIALIZED = 1,
  THREAD_RUNNABLE = 2,
  THREAD_BLOCKED = 3,
  THREAD_RUNNING = 4,
  THREAD_DEAD = 5,
  THREAD_REAPED = 6,
  THREAD_BLOCKED_USER = 7
} ThreadStatus;

#define THREAD_STATUS_CAN_RUN(status) \
    (((status) == THREAD_INITIALIZED) || ((status) == THREAD_RUNNABLE))

// TCB is the kernel presentation of a thread, and is also the unit for
// scheduler
struct _tcb {
  /* BEGIN: main chain cares */
  // id of the thread
  int id;
  // the next tcb in the main chain
  struct _tcb* next;
  /* END: main chain cares */

  // See description above for all states and their transition.
  // Access to it may not be protected by any kmutex / spinlock. The accesser
  // should come up with some atomic operation
  ThreadStatus status;

  /* BEGIN: Section B, only owner of the thread can access to fields in this
  section (owned itself does not, of course)  */

  // the parrent process, is a non-ephemeral access
  pcb* process;
  // the register values before switched out
  ureg_t regs;
  // the start address of kernel stack (with size = PAGE_SIZE)
  uint32_t kernelStackPage;
  // if THREAD_NOT_OWNED, the thread is not owned by anyone
  int owned;
  // bookkeeper for the thread on acquiring itself's memlock. For reentrant lock
  kmutexStatus memLockStatus;
  // When a thread is descheduling (transiting to THREAD_BLOCKED(_USER)),
  // this flag is set until successfully descheduled to prevent further
  // interrupt to schedule to others and leave some staff undone (e.g. reaper)
  bool descheduling;
  // When set, this is the last thread of current process and reaping this will
  // lead to the process to become zombie
  bool lastThread;

  // Set by swexn
  uint32_t faultHandler;
  uint32_t customArg;
  uint32_t faultStack;

  /* BEGIN: Section B */

  CrossCPULock dmlock;    // lock used for makerunnable & deschedule

  // After this line all members should never be used by modules other than
  // process.c
  /* BEGIN: Ephemeral access cares */
  bool _hasAbandoned;
  int _ephemeralRefCount;
  /* END: Ephemeral access cares */

  // XLX cares
  dualLinklist _xlx;
};

#define THREAD_NOT_OWNED -1
#define THREAD_OWNED_BY_CPU 1
#define THREAD_OWNED_BY_THREAD 2


void initProcess();

pcb* findPCB(int pid);
pcb* newPCB();
void removePCB(pcb* proc);
pcb* findPCBWithEphemeralAccess(int pid);
void releaseEphemeralAccessProcess(pcb* proc);

// Ephemeral access is a hint for process module to defer removing.
// Consider the case: the owner of some thread is vanishing, and removing the
// tcb, however, some other cores are getting the TCB pointer and test-and-set
// its owner bit. It's unsafe to free the data structure now.
// As the solution, ephemeral access is introduced for those who are just
// shortly testing the tcb (now the only case is scheduler) but may not really
// own it.
// As soon as the test complete (either test fail, or successfully own it),
// the ephemeral access should be released
tcb* newTCB();
tcb* findTCB(int tid);
tcb* findTCBWithEphemeralAccess(int tid);
void removeTCB(tcb* thread);
// Will always return a runnable thread (maybe the thread passedin itself) due
// to the existence of IDLE thread.
tcb* roundRobinNextTCB(tcb* thread);
// When needToReleaseFormer = true, the former thread will be released
tcb* roundRobinNextTCBWithEphemeralAccess(tcb* thread,
    bool needToReleaseFormer);
void releaseEphemeralAccess(tcb* thread);

void removeFromXLX(tcb* thread);
void addToXLX(tcb* thread);

// for debug
void reportProcessAndThread();

#endif
