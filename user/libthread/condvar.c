/**
 *  @file condvar.c
 *
 *  @brief implementation of conditional variables
 *
 *  The implementation of condvar is very, very similar to mutex (see mutex.c)
 *  They use the same way of handling atomic sleep problem and
 *  They have almost the same member variables, but they differ in thw following
 *  way:
 *  1. Condvar has an embedded mutex to ensure atomicity of lock release (see
 *  cond_wait), so some operations can use normal form instead of atomic op.
 *  2. ticketOnCall means the largest ticket number that has been called, and
 *  all tickets smaller than or equal to this number should be treated as oncall
 *  which is different from mutex where only the ticket numbered ticketOnCall
 *  is treated as oncall
 *  3. spinwait condvar is not a good idea, because the thread is still holding
 *  the external lock, and spinwait may cause deadlock. Therefore, unlike mutex
 *  who will spinwait if there're too many contenders, condvar will directly
 *  crash the current thread with error message outputted.
 *
 *  @author Leiyu Zhao
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <simics.h>
#include <syscall.h>
#include <stddef.h>
#include <mutex.h>
#include <string.h>
#include <cond.h>
#include <thread.h>

int cond_init(cond_t *cv) {
  mutex_init(&cv->mutex);
  cv->nextTicketAvailable = 1;
  cv->ticketOnCall = 0;
  for (int i = 0; i < CONDVAR_MAX_NON_SPINWAIT_THREADS; i++) {
    cv->ticketToTID[i] = -1;
    cv->ticketCalled[i] = 0;
  }
  return 0;
}

void cond_destroy(cond_t *cv) {
  mutex_destroy(&cv->mutex);
  return;
}

void cond_wait(cond_t *cv, mutex_t *mp) {
  mutex_lock(&cv->mutex);
  int myTicket = cv->nextTicketAvailable++;
  int waitSlotForMyTicket = myTicket % CONDVAR_MAX_NON_SPINWAIT_THREADS;
  if (cv->ticketToTID[waitSlotForMyTicket] != -1) {
    mutex_unlock(mp);
    mutex_unlock(&cv->mutex);
    printf("Condvar error since there's not enough block-waiting slot.\n");
    lprintf("Condvar error since there's not enough block-waiting slot.");
    thr_exit(NULL);
  }
  cv->ticketToTID[waitSlotForMyTicket] = gettid();
  mutex_unlock(mp);
  while (true) {
    cv->ticketCalled[waitSlotForMyTicket] = 0;
    // Use memory barrier to force the write order (ticketCalled=0 first)
    __sync_synchronize();
    if (cv->ticketOnCall >= myTicket) break;
    mutex_unlock(&cv->mutex);
    deschedule(&cv->ticketCalled[waitSlotForMyTicket]);
    mutex_lock(&cv->mutex);
  }

  cv->ticketCalled[waitSlotForMyTicket] = 1;
  cv->ticketToTID[waitSlotForMyTicket] = -1;
  mutex_unlock(&cv->mutex);
  mutex_lock(mp);
}

void cond_signal(cond_t *cv) {
  mutex_lock(&cv->mutex);
  if (cv->nextTicketAvailable == cv->ticketOnCall + 1) {
    // No one is waiting
    mutex_unlock(&cv->mutex);
    return;
  }
  cv->ticketOnCall++;

  __sync_synchronize();

  int myNextTicketWaitSlot = cv->ticketOnCall % CONDVAR_MAX_NON_SPINWAIT_THREADS;
  cv->ticketCalled[myNextTicketWaitSlot] = 1;
  int nextTicketHolderLocal = cv->ticketToTID[myNextTicketWaitSlot];
  if (nextTicketHolderLocal >= 0) {
    make_runnable(nextTicketHolderLocal);
  }
  mutex_unlock(&cv->mutex);
}

void cond_broadcast(cond_t *cv) {
  mutex_lock(&cv->mutex);
  if (cv->nextTicketAvailable == cv->ticketOnCall + 1) {
    // No one is waiting
    mutex_unlock(&cv->mutex);
    return;
  }
  int oldOnCall = cv->ticketOnCall;
  cv->ticketOnCall = cv->nextTicketAvailable - 1;

  __sync_synchronize();

  for (int tik = oldOnCall+1; tik <= cv->ticketOnCall; tik++) {
    int tikSlot = tik % CONDVAR_MAX_NON_SPINWAIT_THREADS;
    cv->ticketCalled[tikSlot] = 1;
    int nextTicketHolderLocal = cv->ticketToTID[tikSlot];
    if (nextTicketHolderLocal >= 0) {
      make_runnable(nextTicketHolderLocal);
    }
  }
  mutex_unlock(&cv->mutex);
}
