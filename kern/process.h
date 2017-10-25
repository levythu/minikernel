/** @file process.h
 *
 *  @brief Core data structure for PCB and TCB.
 *
 *  The description of TCB and PCB are detailed below; Besides the definition,
 *  this module only manages the internal storage of TCB/PCB itself. To specify,
 *  it's add/delete/find, and a global lock is used to only protect its present
 *  data sturcture.
 *
 *  Besides those, this module is NOT RESPONSIBLE for any synchronization and
 *  race-prevention between different user of tcb/pcb.
 *
 *  TCB and PCB are kept in two linked list.
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
#include "kmutex.h"

typedef struct _pcb {
  int id;
  struct _pcb* next;
  // Read only after creation
  ProcessMemoryMeta memMeta;
  PageDirectory pd;
  int parentPID;

  // used to protect member access below me
  // Also, PageDirectory minor changes (new_pages) should be serial
  // However, initializing phase, fork and exec don't need its protection, since
  // they can only be called when the process only have one thread
  kmutex mutex;
  int numThread;

  // Use to protect page table access, including accessing user space memory,
  // because kernel never knows when another thread of the process remove the
  // mem mapping without locking it.
  // It's used as a read-write lock, with all memory access as reading, and page
  // directoy modification as writing (actually, R/W on page table)
  kmutex memlock;
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

// TCB is the kernel presentation of a thread, and is also the unit for
// scheduler
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
