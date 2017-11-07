/**
 *  @file mutex.c
 *
 *  @brief Implementation of Mutex
 *
 *  This implementation mainly adopts block-waiting version of ticket-based
 *  mutex, while making extra tradeoff between the possibility of spinlock under
 *  heavy contention and space.
 *
 *  Generally, each thread who tries to acquire the lock will get a unique
 *  increasing ticket (this increasing number is got from nextTicketAvailable ).
 *  Then it puts his TID into ticketToTID for the others to wake and keeps
 *  sleeping until ticketOnCall is equal to own ticket.
 *  ticketCalled is introduced to eliminate race condition. The thread will only
 *  sleep when its ticketCalled value is 0. The waker will set it to 1 first
 *  before trying to wake it up.
 *
 *  Since this implementation needs every participant (block-waiting) thread to
 *  have its private ticketCalled and ticketToTID, we don't want to reserve the
 *  space for max number of possible contending threads. So we introduce
 *  MUTEX_MAX_NON_SPINWAIT_THREADS, and the ticket numer will mod it to get
 *  its block-waiting slot (ticketCalled/ticketToTID space).
 *  If this one is taken by someone else (happens when the number of waiting
 *  threads exceeds MUTEX_MAX_NON_SPINWAIT_THREADS), it keeps spinning until
 *  the place is spare. While spinlock is bad, this only happens when there're
 *  really a lot of threads contending the lock.
 *
 *  Uses several GCC builtin atmoic operations learnt from 15-418 last semester:
 *  https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
 *
 *  @author Leiyu Zhao
 *
 */

#include <stdint.h>
#include <simics.h>
#include <syscall.h>
#include <stddef.h>
#include <mutex.h>
#include <string.h>


// Fetch the next ticket number. This method is thread safe.
static int fetch_next_ticket(mutex_t *mp) {
  return __sync_fetch_and_add(&mp->nextTicketAvailable, 1);
}

int mutex_init(mutex_t *mp) {
  mp->nextTicketAvailable = 0;
  mp->ticketOnCall = 0;
  for (int i = 0; i < MUTEX_MAX_NON_SPINWAIT_THREADS; i++) {
    mp->ticketToTID[i] = -1;
    mp->ticketCalled[i] = 0;
  }
  return 0;
}

void mutex_destroy(mutex_t *mp) {
  return;
}

void mutex_lock(mutex_t *mp) {
  // 1. Get a ticket, then spinlock to get into the block-waiting list
  // Spinlock should very rarely happens since MUTEX_MAX_NON_SPINWAIT_THREADS
  // is large enough to cover common case of lock contention.
  int myTicket = fetch_next_ticket(mp);
  int waitSlotForMyTicket = myTicket % MUTEX_MAX_NON_SPINWAIT_THREADS;
  while (myTicket - mp->ticketOnCall >= MUTEX_MAX_NON_SPINWAIT_THREADS) {
    ; // Wait until myTicket is within the block-waiting window
  }
  mp->ticketToTID[waitSlotForMyTicket] = gettid();

  // 2. Now we are in block-waiting list, since ticketToTID[waitSlotForMyTicket]
  // is storing my TID.
  while (true) {
    mp->ticketCalled[waitSlotForMyTicket] = 0;
    // Use memory barrier to force the write order (ticketCalled=0 first)
    __sync_synchronize();
    if (mp->ticketOnCall == myTicket) break;
    deschedule(&mp->ticketCalled[waitSlotForMyTicket]);
  }

  // 3. Whooooo! I'm in critical section now! Remove myself from block-waiting
  // list so that others can take this position ASAP
  mp->ticketCalled[waitSlotForMyTicket] = 1;
  mp->ticketToTID[waitSlotForMyTicket] = -1;
}

void mutex_unlock(mutex_t *mp) {
  // 1. Rolling the oncall.
  int myNextTicket = __sync_add_and_fetch(&mp->ticketOnCall, 1);
  // Use memory barrier to force the write order (inc mp->ticketOnCall first)
  __sync_synchronize();

  // 2. From this point on, we are not in critical section anymore
  // We want to optionally wake up the guy holding the next ticket. This action
  // is always safe: even if we wake up somebody interleaving inside by mistake
  // the absolute ticketOnCall will force the bad guy back to sleep again!
  int myNextTicketWaitSlot = myNextTicket % MUTEX_MAX_NON_SPINWAIT_THREADS;
  mp->ticketCalled[myNextTicketWaitSlot] = 1;
  int nextTicketHolderLocal = mp->ticketToTID[myNextTicketWaitSlot];
  if (nextTicketHolderLocal >= 0) {
    make_runnable(nextTicketHolderLocal);
  }
}
