/**
 *  @file sem.c
 *
 *  @brief Implementation of semaphore
 *
 *  Semaphore is implemented on mutex and condvar and is quite simple. Unlike
 *  RWLock, we only have one waiting queue here so we don't have to keep tickets
 *  manually: condvar can help us do that.
 *  When there's enough resources, fetch it without blocking (not subscribe
 *  condvar either); When there's no resource, subscribe to condvar and wait to
 *  be called.
 *  - mutex is the mutex accessing semaphore
 *  - cond is the queue for waiter, each time someone release a resource, one
 *    member subscribing to the condvar will be awakened
 *  - resourceCount: the number of resources left for the semaphore
 *
 *  @author Leiyu Zhao
 *
 */

#include <stdint.h>
#include <simics.h>
#include <syscall.h>
#include <stddef.h>
#include <sem.h>
#include <string.h>

int sem_init(sem_t *sem, int count) {
  mutex_init(&sem->mutex);
  cond_init(&sem->cond);
  sem->resourceCount = count;
  return 0;
}

void sem_destroy(sem_t *sem) {
  mutex_destroy(&sem->mutex);
  cond_destroy(&sem->cond);
}

void sem_wait(sem_t *sem) {
  mutex_lock(&sem->mutex);
  if (sem->resourceCount == 0) {
    // Push myself into the waiter of cond var, and wait for call
    while (true) {
      cond_wait(&sem->cond, &sem->mutex);
      if (sem->resourceCount > 0) break;
    }
  }
  sem->resourceCount--;
  mutex_unlock(&sem->mutex);
}

void sem_signal(sem_t *sem) {
  mutex_lock(&sem->mutex);
  sem->resourceCount++;
  cond_signal(&sem->cond);
  mutex_unlock(&sem->mutex);
}
