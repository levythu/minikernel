/** @file TODO
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

typedef enum {
  HyperNA = 0,
  HyperNew = 1,
  HyperInited = 2
} HyperStatus;

#define HYPER_STATUS_READY(s) ((s) != HyperNA && (s) != HyperNew)

typedef struct HyperInfo {
  bool isHyper;
  HyperStatus status;
  int cs;
  int ds;
  // Replicate cs/ds's base addr
  uint32_t baseAddr;

  // After it, will not be set by loader
  bool interrupt;

  // For those who want to send interrupt to this hyper only, broadcast this
  // This helps prevent race when this hypervisor is exiting
  // (idt and delayedInt are vanishing)
  intMultiplexer selfMulti;

  // The following two are protected by GlobalLock, and also they are the only
  // fields accessible by something outside hypersivor
  IDTEntry* idt;
  varQueue delayedInt;
  CrossCPULock latch;

} HyperInfo;

void initHyperInfo(HyperInfo* info);

// Return true if given elfMetadata is a virtual machine.
// In this case, modify elfMetadata to apply proper offset, and set HyperInfo
// Otherwise, elfMetadata is unchanged, with normal HyperInfo
bool fillHyperInfo(simple_elf_t* elfMetadata, HyperInfo* info);

void destroyHyperInfo(HyperInfo* info);

void bootstrapHypervisorAndSwitchToRing3(
    HyperInfo* info, uint32_t entryPoint, uint32_t eflags);

#endif
