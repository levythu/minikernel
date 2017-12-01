#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <hvcall_int.h>
#include <hvcall.h>
#include <x86/asm.h>

#include "int_handler.h"
#include "common_kern.h"
#include "bool.h"
#include "process.h"
#include "cpu.h"
#include "hv.h"
#include "hvlife.h"
#include "source_untrusted.h"
#include "hv_hpcall_internal.h"
#include "scheduler.h"
#include "zeus.h"
#include "vm.h"


// clear the current pd's user space
// If it's the same as backed pd, then it's just a shallow clear
// Otherwise it's a deep clear (free PT)
static void clearOriginalPD(tcb* thr) {
  HyperInfo* info = thr->process->hyperInfo;
  PageDirectory pd = thr->process->pd;
  assert(info->originalPD != NULL);
  for (int i = STRIP_PD_INDEX(USER_MEM_START); i <= STRIP_PD_INDEX(0xffffffff);
      i++) {
    if (pd[i] = info->originalPD[i]) {
      pd[i] = EMPTY_PDE;
    } else {
      if (PE_IS_PRESENT(pd[i])) {
        sfree(PDE2PT(pd[i]), sizeof(PTE) * PT_SIZE);
      }
      pd[i] = EMPTY_PDE;
    }
  }
}

// guestPAddr = 0xffffffff for remove,
// return true for success
static bool generatePDMapping(tcb* thr, bool isWritable, bool isUser,
    uint32_t guestVAddr, uint32_t guestPAddr, bool forceRefresh) {
  HyperInfo* info = thr->process->hyperInfo;
  PageDirectory pd = thr->process->pd;

  if (guestVAddr > GUEST_PHYSICAL_MAXVADDR) {
    return false;
  }
  uint32_t hostVAddr = guestVAddr + info->baseAddr;
  assert(hostVAddr >= USER_MEM_START);
  if (guestPAddr == 0xffffffff ||
      ()) {
    // remove mapping
    PTE* currentPTE = searchPTEntryPageDirectory(pd, hostVAddr);
    if (!currentPTE) {
      // already nonexist. Good.
      return true;
    }
    *currentPTE &= ~PE_PRESENT(1);
  } else {

  }

  info->originalPD
}

// Will discard the current mapping
// We don't fear that interrupt may use guest memory: that only happens when
// iret will directly going back.
// Failure may leave page directory in a total mess
// pdGuestAddr must has already applied segment offset
// guestPD must be a kernel space guest PD
static bool reCompileGuestPD(tcb* thr, PageDirectory guestPD) {
  // clear current page table
  clearOriginalPD(thr);

  // compile
  for (int i = 0; i < PD_SIZE; i++) {
    // TODO validate unneeded bits
    if (!PE_IS_PRESENT(guestPD[i])) continue;
    bool isUserRoot = PE_IS_USERMODE(guestPD[i]);
    bool isWritableRoot = PE_IS_WRITABLE(guestPD[i]);
    PageTable cpt = PDE2PT(guestPD[i]);
    for (int j = 0; j < PT_SIZE; j++) {
      if (!PE_IS_PRESENT(cpt[j])) continue;
      bool isUser = isUserRoot && PE_IS_USERMODE(cpt[j]);
      bool isWritable = isWritableRoot && PE_IS_WRITABLE(cpt[j]);
      // TODO validate unneeded bits

    }
  }
}

// shallow - backing up the current pd
static void backupOriginalPD(tcb* thr) {
  HyperInfo* info = thr->process->hyperInfo;
  PageDirectory pd = thr->process->pd;
  assert(info->originalPD == NULL);
  info->originalPD = newPageDirectory();
  for (int i = STRIP_PD_INDEX(USER_MEM_START); i <= STRIP_PD_INDEX(0xffffffff);
      i++) {
    info->originalPD[i] = pd[i];
  }
}

// Will discard current guest page directory
static void reActivateOriginalPD(tcb* thr) {
  HyperInfo* info = thr->process->hyperInfo;
  PageDirectory pd = thr->process->pd;
  for (int i = STRIP_PD_INDEX(USER_MEM_START); i <= STRIP_PD_INDEX(0xffffffff);
      i++) {
    if (PE_IS_PRESENT(pd[i])) {
      sfree(PDE2PT(pd[i]), sizeof(PTE) * PT_SIZE);
    }
    pd[i] = info->originalPD[i];
  }
  // Force a context switch to ensure directory change and relavidation
  // (Given the assumtion that a hypervisor only has one thread)
  assert(yieldToNext());
}

int hpc_magic(int userEsp, tcb* thr) {
  HyperInfo* info = thr->process->hyperInfo;
  if (!info->originalPD) {
    backupAndClearOriginalPD(thr);
  }
  // TODO copy the guest pd

}
