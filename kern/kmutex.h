/** @file kmutex.h
 *
 *  @brief TODO
 *
 *  This module
 *
 *  @author Leiyu Zhao
 */

#ifndef KMUTEX_H
#define KMUTEX_H

#include <stdlib.h>

#include "bool.h"
#include "cpu.h"
#include "sysconf.h"
#include "process.h"

typedef struct _waiterLinklist {
  tcb* thread;
  struct _waiterLinklist* next;
} waiterLinklist;

typedef struct {
  CrossCPULock spinMutex;
  waiterLinklist* queue;
  bool available;
} kmutex;

void kmutexInit(kmutex* km);
void kmutexLock(kmutex* km);
void kmutexUnlock(kmutex* km);

#endif
