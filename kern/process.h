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
#include "cpu.h"

typedef struct _pcb {
  int id;
  struct _pcb* next;
  // Read only after creation
  ProcessMemoryMeta memMeta;
  PageDirectory pd;
  int parentPID;

  // Changable
  int numThread;
  // TODO access control
} pcb;

typedef enum ThreadStatus {
  THREAD_UNINITIALIZED = 0,
  THREAD_INITIALIZED = 1,
  THREAD_RUNNABLE = 2,
  THREAD_BLOCKED = 3,
  THREAD_RUNNING = 4
} ThreadStatus;

#define THREAD_STATUS_CAN_RUN(status) \
    (((status) == THREAD_INITIALIZED) || ((status) == THREAD_RUNNABLE))

typedef struct _tcb {
  int id;
  struct _tcb* next;

  // This member is only interested by context switch
  ThreadStatus status;
  pcb* process;
  ureg_t regs;
  uint32_t kernelStackPage;
  int owned;
} tcb;

#define THREAD_NOT_OWNED -1
#define THREAD_OWNED_BY_CPU 1
#define THREAD_OWNED_BY_THREAD 2

void initProcess();

pcb* findPCB(int pid);
pcb* newPCB();

tcb* newTCB();
tcb* findTCB(int tid);
tcb* roundRobinNextTCBID(int tid);
tcb* roundRobinNextTCB(tcb* thread);

#endif
