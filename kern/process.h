/** @file process.h
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#ifndef PROCESS_H
#define PROCESS_H

#include "sysconf.h"
#include "vm.h"
#include "loader.h"
#include "ureg.h"

typedef struct _pcb {
  int id;
  struct _pcb* next;
  ProcessMemoryMeta memMeta;
  PageDirectory pd;
} pcb;

typedef struct _tcb {
  int id;
  struct _tcb* next;
  pcb* process;
  ureg_t regs;
} tcb;

void initProcess();

pcb* findPCB(int pid);
pcb* newPCB();

tcb* newTCB();
tcb* findTCB(int tid);

#endif
