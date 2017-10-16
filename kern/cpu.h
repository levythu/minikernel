/** @file cpu.h
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#ifndef CPU_H
#define CPU_H

#include "bool.h"

typedef struct {
  int id;
  bool interruptSwitch;
  int runningTID;
  int runningPID;

  int currentMutexLayer;
} cpu;

typedef struct {
  int holderCPU;
  int currentMutexLayer;
} CrossCPULock;

void initCPU();

cpu* getLocalCPU();

void DisableInterrupts();
void EnableInterrupts();

void LocalLockR();
void LocalUnlockR();

void initCrossCPULock(CrossCPULock* lock);
void GlobalLockR(CrossCPULock* lock);
void GlobalUnlockR(CrossCPULock* lock);

#endif
