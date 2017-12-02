/** @file hvinterrupt.h
 *
 *  @brief The key module for virtual interrupt delivery
 *
 *  Virtual interrupts are either
 *  1. Immediate interrupt: trap (INT ??) or exception (IDT 0~19)
 *  2. Delayed interrupt: hardware interrupt (keyboard, timer)
 *
 *  To avoid using any locks, we ensure a key condition:
 *  **All interrupt deliveries will not interleave**
 *
 *  We ensure this by two ways:
 *  1. We only deliver interrupts when it's on its own process space (i.e. when
 *     interrupt happens in other threads, we only records them)
 *  2. We only deliver interrupts when we are only one-iret way from guest.
 *     (i.e., one iret will go to guest. this is achieved by examine %cs on iret
 *     stack)
 *
 *  This design limit the speed of guest to consume virtual interrupt, so that
 *  only on one timer tick (or guest make hypercalls/exceptions) there can be
 *  one virtual interrupt consumed. For immediate interupt it's okay: we are
 *  just-in-pace; but for delayed interrupt, when it's generated too fast,
 *  the delayed int queue runs out. That's why we keep virtual timer rate at
 *  2 per own timer tick (timer tick when it's on my own process)
 *
 *  So how to notify hypervisor of int? The first way is pushevent (see
 *  pushevent.c), so that outsider is hooked to call some functions provided;
 *  The second way is via multiplexer (see below), outsider will notify
 *  multiplexer anyway, and live virtual machine will register itself to
 *  multiplexer and listen to those event
 *
 *  @author Leiyu Zhao
 */

#ifndef HV_INTERRUPT_H
#define HV_INTERRUPT_H

#include <stdint.h>

#include "kmutex.h"
#include "bool.h"

// Max idtnumber
#define MAX_SUPPORTED_VIRTUAL_INT 134
// The size of delayedInt queue
#define MAX_WAITING_INT 512
// Can be set to quite large, which decide the size of intMultiplexer
#define MAX_CONCURRENT_VM 8

// Basic struct for a virtual interrupt, with its idtNumber (intNum), special
// code (error code for excpetions, augChar for key interrupt and 0 for others)
// and cr2 (only valid for page fault)
typedef struct {
  int intNum;
  int spCode;
  int cr2;
} hvInt;

// The IDT entry registered to a hypervisor
typedef struct {
  // flag is used to ensure data integrity, so pay attention to the order of
  // setting data
  bool present;
  uint32_t eip;
  bool privileged;
} IDTEntry;

// Append an delayed int to one hypervisor, returns false if IDT is not
// specified or int queue is full
// Info is actually a hyperInfo*
bool appendIntTo(void* info, hvInt hvi);

/*****************************************************************************/
// Interrupt Multiplexer:
// Multiplexer is used to multiplex the delayed int deliver process. Kernel
// modules will keep their own multiplexer, and hypervisor go to register it.
// (e.g. virtual keyboard)
// Multiplexer is designed to be multithread-safe so that it can be called from
// interrupt handler.

typedef struct {
  void* waiter[MAX_CONCURRENT_VM];    // HyperInfo*
  kmutex mutex;
} intMultiplexer;

// Initiate a multiplexer, the owner should do that
void initMultiplexer(intMultiplexer* mper);

// Broadcast a virtual interrupt to a multiplexer
void broadcastIntTo(intMultiplexer* mper, hvInt hvi);

// Register a hypervisor (hyperInfo*) to the multiplexer. Will panic if
// listener > MAX_CONCURRENT_VM. So keep MAX_CONCURRENT_VM large enough
void addToWaiter(intMultiplexer* mper, void* info);

// Deregister a hypervisor (hyperInfo*) from the multiplexer
// Given hypervisor must be a listener of it.
void removeWaiter(intMultiplexer* mper, void* info);

#endif
