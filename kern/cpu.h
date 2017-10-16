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
  bool interruptSwitch;
  int runningTID;
  int runningPID;
} cpu;

void initCPU();

cpu* getLocalCPU();

void DisableInterrupts();
void EnableInterrupts();

#endif
