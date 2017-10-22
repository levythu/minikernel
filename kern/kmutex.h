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

typedef struct _waiterLinklist {
  void* thread;   // tcb, but to avoid circlular dependancy
  struct _waiterLinklist* next;
} waiterLinklist;

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

#endif
