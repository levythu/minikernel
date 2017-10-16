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

typedef struct {
  int id;
  void* next;
} cb;

typedef struct _pcb {
  int id;
  struct _pcb* next;
  PageDirectory pd;
} pcb;

typedef struct _tcb {
  int id;
  struct _tcb* next;
  pcb* process;
} tcb;

#endif
