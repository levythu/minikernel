/** @file hvlife.h
 *
 *  @brief Controlling the life time for a hypervisor
 *
 *  A hypervisor's lifeline is like this:
 *  - When a normal program calls exec() on a hypervisor, it enters hypervisor
 *    mode, initializing HyperInfo and set status = HyperNew (loader calling
 *    fillHyperInfo )
 *  - Instead of just entering ring3, exec() will call
 *    bootstrapHypervisorAndSwitchToRing3 to complete the whole init phase for
 *    a hypervisor and entering guest. At this point, status = HyperInited
 *  - When a process decides to exit, it calls exitHyperWithStatus (instead of
 *    Zeus::terminateThread)
 *
 *  @author Leiyu Zhao
 */

#ifndef HVLIFE_H
#define HVLIFE_H

#include <stdint.h>
#include <elf_410.h>

#include "bool.h"
#include "hvseg.h"
#include "hvinterrupt.h"
#include "cpu.h"
#include "queue.h"
#include "vm.h"

typedef enum {
  HyperNA = 0,
  HyperNew = 1,
  HyperInited = 2
} HyperStatus;

#define HYPER_STATUS_READY(s) ((s) != HyperNA && (s) != HyperNew)

// The basic data structure describing a hypervisor, which is embedded in
// PCB
typedef struct HyperInfo {
  // === SectionA: Fixed after HyperNew status
  // True for a hypervisor
  bool isHyper;

  // The status of hypervisor, HyperNA for a non-hypervisor
  // For its states and trasition see comments above
  HyperStatus status;

  // The cs of the hyper
  int cs;

  // The ds of the hyper
  int ds;

  // Replicate cs/ds's base addr
  uint32_t baseAddr;

  // === SectionA ends


  // Except SectionA, any field will not ready until status == HyperInited


  // === SectionB: Fields that will only be accessed in self's one-iret away
  //               kernel stack
  // Can delayed interrupt be delivered
  bool interrupt;

  // For those who want to send interrupt to this hyper only, broadcast this
  // This helps prevent race when this hypervisor is exiting
  // (idt and delayedInt are vanishing)
  intMultiplexer selfMulti;

  // The number of own timer ticks (see hvinterrupt.h)
  uint32_t tics;

  // === SectionB ends


  // === SectionC: Fields that may be accessed in multithreaded or reentry way
  //               Therefore the section are protected by latch

  // The idt table
  IDTEntry* idt;

  // the delayed int queue
  varQueue delayedInt;
  CrossCPULock latch;
  // === SectionC ends


  // === SectionD: Fields that will only be accessed in self's one-iret away
  //               kernel stack (as SectionB). It's all about paging & Umode

  // If null, paging is off;
  // Otherwise, point to one shallow copy of the original direct-map page table
  // This is a copied table, so destructor should release it when unnecessary
  PageDirectory originalPD;

  // virtual CR3. (before + segment offset)
  // When == 0, page is off
  uint32_t vCR3;

  // can guest write to readonly pages in ring0
  bool writeProtection;

  // whether the guest is in ring0
  bool inKernelMode;

  // The stack for next switch-to-ring0
  uint32_t esp0;
  // === SectionD ends

} HyperInfo;

// Setup sane value for a hyperinfo.
void initHyperInfo(HyperInfo* info);

// Return true if given elfMetadata is a virtual machine.
// In this case, modify elfMetadata to apply proper offset, and set HyperInfo
// Otherwise, elfMetadata is unchanged, with normal HyperInfo
bool fillHyperInfo(simple_elf_t* elfMetadata, HyperInfo* info);

// One-way function, it's to a hypervisor is like how terminateThread is to
// a normal program
void exitHyperWithStatus(HyperInfo* info, void* thr, int statusCode);

// Finish all initialization of hypervisor, and go into guest
void bootstrapHypervisorAndSwitchToRing3(
    HyperInfo* info, uint32_t entryPoint, uint32_t eflags, int vcn);

#endif
