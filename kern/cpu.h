/** @file cpu.h
 *
 *  The module for managing CPUs.
 *
 *  What we are trying to achieve is that all CPU-number dependent stuff are
 *  kept inside the module, so that scaling to multicore does not hurt too much.
 *
 *  Generally, it has several things to do:
 *  - bookkeeping CPU: Record the current info for CPU, e.g., current thread,
 *        current process, etc..
 *  - multicore interface: provides method to manipulate "current core", to
 *        hide multicore from outsiders
 *  - concurrency: enable two locks:
 *      - LocalLock will disable current CPU's interrupt, so when holding the
 *        lock, current CPU cannot run another thread to enter the critical
 *        section
 *      - GlobalLock will exclude all the other cores from entering the c.s.,
 *        of course, it is a superset of LocalLock. GlobalLock is implemented
 *        by spinning, but in single core, spinning never happens.
 *    all the locks are re-entrant, i.e. the same CPU can acquire the lock more
 *    than once.
 *
 *  @author Leiyu Zhao
 */

#ifndef CPU_H
#define CPU_H

#include "bool.h"

// CPU record, id, runningPID and runningTID make sense to the outsiders
typedef struct {
  int id;
  bool interruptSwitch;
  int runningTID;
  int runningPID;

  int currentMutexLayer;
} cpu;

// GlobalLock object.
typedef struct {
  int holderCPU;
  int currentMutexLayer;
} CrossCPULock;

// Initialize the state for CPU, should be called exactly once on bootstrap.
void initCPU();

// Get the current CPU. Since now it's only single core machine, this will
// always return that CPU.
cpu* getLocalCPU();

// The Wrapper of DisableInterrupts and Enable. Be SURE to use it after init
// CPU
void DisableInterrupts();
void EnableInterrupts();

// re-entrant local lock api
void LocalLockR();
void LocalUnlockR();

// re-entrant global lock api, and the lock object
void initCrossCPULock(CrossCPULock* lock);
void GlobalLockR(CrossCPULock* lock);
void GlobalUnlockR(CrossCPULock* lock);

#endif
