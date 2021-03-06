/** @file kmutex.h
 *
 *  @brief kernel blocking mutex
 *
 *  The blocking mutex differs from CrossCPULock in cpu.c in that this one is
 *  blocking, instead of spinlock. It deschedule current thread when waiting for
 *  lock.
 *
 *  @author Leiyu Zhao
 */

#ifndef KMUTEX_H
#define KMUTEX_H

#include <stdlib.h>

#include "bool.h"
#include "cpu.h"
#include "sysconf.h"

typedef struct _waiterLinklist {
  void* thread;   // tcb, but to avoid circlular dependancy
  struct _waiterLinklist* next;
} waiterLinklist;

typedef enum _kmutexStatus {
  KMUTEX_NOT_ACQUIRED = 0,
  KMUTEX_HAVE_RLOCK = 1,
  KMUTEX_HAVE_WLOCK = 2,
} kmutexStatus;

typedef struct _kmutex {
  CrossCPULock spinMutex;
  waiterLinklist* readerWL;
  waiterLinklist* writerWL;
  int status;
} kmutex;

void kmutexInit(kmutex* km);
void kmutexRLock(kmutex* km);
void kmutexRUnlock(kmutex* km);
void kmutexWLock(kmutex* km);
void kmutexWUnlock(kmutex* km);

// This set of APIs do essentially the same as above, while bookkeeping the
// current states into status atomically.
// The state can be used for kmutexWLockForce
void kmutexRLockRecord(kmutex* km, kmutexStatus* status);
void kmutexRUnlockRecord(kmutex* km, kmutexStatus* status);
void kmutexWLockRecord(kmutex* km, kmutexStatus* status);
void kmutexWUnlockRecord(kmutex* km, kmutexStatus* status);

// Special API, it's recursive: one thread can call this even if getting WLock
// It's adaptive: one can call it with either NoLock or WLock (RLock is not
// permitted!) and unlock force can recover it.
void kmutexWLockForce(kmutex* km, kmutexStatus* status,
    kmutexStatus* statusToRestore);
void kmutexWUnlockForce(kmutex* km, kmutexStatus* status,
    kmutexStatus statusToRestore);

#endif
