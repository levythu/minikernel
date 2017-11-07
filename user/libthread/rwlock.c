/**
 *  @file rwlock.c
 *
 *  @brief implementation of read write lock.
 *
 *  It's implementation is actually quite bruteforce: similar to condvar and
 *  mutex, it still uses ticket assigning, but since now we can use condvar and
 *  mutex, we don't have to implement a lot of complex codes to ensure semantics
 *
 *  RWLock has two ticket queues, one for read and one for write. When a reader
 *  (or write) cannot acquire immediately, it fetches a ticket from correspond-
 *  ing queue, and subscribe the alarm (a condvar)
 *
 *  When there's any new ticket called, alarm is broadcasted so everyone can
 *  wake up to check their number. As you can see, it's not most efficient, but
 *  it's easy and less vulnerable to bugs
 *
 *  This implemementation solve the 2nd readwrite problem. When there's any
 *  pending writers, readers cannot get in. It is implemented by prioritize
 *  calling number from writer queue.
 *
 *  Member of rwlock:
 *  - state: Lock state of the lock. If =0, the lock is free; if >0, the lock
 *        is read-locked and the number indicates the number of readers; if -1,
 *        the lock is write-locked
 *  - mutex: the mutex. any access to rwlock will be protected by mutex
 *  - alarm: the condvar, which used for subscription so that any change will
 *        notify everyone
 *  - nextAvailableWriterTicket: the next write ticket that can be assigned
 *  - writerTicketOnCall: all the write ticket <= this number has been called.
 *        it must < nextAvailableWriterTicket
 *  - readerTicketOnCall / nextAvailableReaderTicket: same as writerTicketOnCall
 *        and nextAvailableWriterTicket
 *
 *  @author Leiyu Zhao
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <simics.h>
#include <syscall.h>
#include <stddef.h>
#include <stdlib.h>
#include <mutex.h>
#include <string.h>
#include <cond.h>
#include <thread.h>
#include <rwlock.h>

int rwlock_init(rwlock_t *rwlock) {
  mutex_init(&rwlock->mutex);
  cond_init(&rwlock->alarm);
  rwlock->state = 0;
  rwlock->nextAvailableWriterTicket = 0;
  rwlock->nextAvailableReaderTicket = 0;
  rwlock->writerTicketOnCall = -1;
  rwlock->readerTicketOnCall = -1;
  return 0;
}

void rwlock_destroy(rwlock_t *rwlock) {
  mutex_destroy(&rwlock->mutex);
  cond_destroy(&rwlock->alarm);
}

// Aquire the rlock.
static void rlock(rwlock_t *rwlock) {
  mutex_lock(&rwlock->mutex);
  if (rwlock->state >= 0 &&
      rwlock->writerTicketOnCall+1 == rwlock->nextAvailableWriterTicket) {
    // The lock is free/rlocked, and there's no writer waiting, then
    // just increase the read count and we are good to go.
    rwlock->state++;
    mutex_unlock(&rwlock->mutex);
    return;
  }
  // Otherwise, we need to fetch a read ticket, and subscribe myself to alarm
  // so that I can be notified when it's good
  int myTicket = rwlock->nextAvailableReaderTicket++;
  while (myTicket > rwlock->readerTicketOnCall) {
    cond_wait(&rwlock->alarm, &rwlock->mutex);
  }
  if (rwlock->state < 0) {
    panic("It's impossible! When I'm oncall no writer should hold it!");
  }
  rwlock->state++;
  mutex_unlock(&rwlock->mutex);
}

// Acquire the wlock
static void wlock(rwlock_t *rwlock) {
  mutex_lock(&rwlock->mutex);
  if (rwlock->state == 0) {
    // The lock is free, we are good to go.
    rwlock->state = -1;
    mutex_unlock(&rwlock->mutex);
    return;
  }
  // Otherwise, we need to fetch a write ticket, and subscribe myself to alarm
  // so that I can be notified when it's good
  int myTicket = rwlock->nextAvailableWriterTicket++;
  while (myTicket > rwlock->writerTicketOnCall) {
    cond_wait(&rwlock->alarm, &rwlock->mutex);
  }
  if (rwlock->state != 0) {
    panic("It's impossible! When I'm oncall no one else should hold it!");
  }
  rwlock->state = -1;
  mutex_unlock(&rwlock->mutex);
}

// Based on type, aquire a lock
void rwlock_lock(rwlock_t *rwlock, int type) {
  if (type == RWLOCK_READ) {
    rlock(rwlock);
  } else {
    wlock(rwlock);
  }
}

void rwlock_unlock(rwlock_t *rwlock) {
  mutex_lock(&rwlock->mutex);
  // We judge what to do based on the state
  if (rwlock->state == 0) {
    panic("Release a free lock, Are you serious?");
  }
  if (rwlock->state > 0) {
    // Readlock, decrease it by one
    rwlock->state--;
  } else {
    // Writelock, free it
    rwlock->state = 0;
  }
  if (rwlock->state == 0) {
    // Whooo, we are good to assign new holder!
    if (rwlock->writerTicketOnCall+1 < rwlock->nextAvailableWriterTicket) {
      // someone is waiting to get a wlock. call the ticket
      rwlock->writerTicketOnCall++;
    } else {
      // no writer is here, every reader, no time to explain. Get aboard!
      rwlock->readerTicketOnCall = rwlock->nextAvailableReaderTicket - 1;
    }
    // everyone, wake up!
    cond_broadcast(&rwlock->alarm);
  }
  mutex_unlock(&rwlock->mutex);
}

void rwlock_downgrade(rwlock_t *rwlock) {
  mutex_lock(&rwlock->mutex);
  if (rwlock->state != -1) {
    panic("Hey dude, what are you doing!?");
  }
  // convert the state to rlock, and allow all waiting readers to go
  rwlock->state = 1;
  rwlock->readerTicketOnCall = rwlock->nextAvailableReaderTicket - 1;
  cond_broadcast(&rwlock->alarm);
  mutex_unlock(&rwlock->mutex);
}
