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
static void clearCurrentPD(tcb* thr) {
  HyperInfo* info = &thr->process->hyperInfo;
  PageDirectory pd = thr->process->pd;
  assert(info->originalPD != NULL);
  for (int i = STRIP_PD_INDEX(USER_MEM_START); i <= STRIP_PD_INDEX(0xffffffff);
      i++) {
    if (pd[i] != info->originalPD[i]) {
      if (PE_IS_PRESENT(pd[i])) {
        sfree(PDE2PT(pd[i]), sizeof(PTE) * PT_SIZE);
      }
    }
    pd[i] = EMPTY_PDE;
  }
}

// guestPAddr = 0xffffffff for remove,
// return true for success
static bool generatePDMapping(tcb* thr, bool isWritable, bool isUser,
    uint32_t guestVAddr, uint32_t guestPAddr, bool forceRefresh) {
  HyperInfo* info = &thr->process->hyperInfo;
  PageDirectory pd = thr->process->pd;

  if (guestVAddr > GUEST_PHYSICAL_MAXVADDR) {
    return false;
  }
  if (guestPAddr >= HYPERVISOR_MEMORY && guestPAddr != 0xffffffff) {
    return false;
  }
  uint32_t hostVAddr = guestVAddr + info->baseAddr;
  assert(hostVAddr >= USER_MEM_START);
  if (guestPAddr == 0xffffffff ||
      (!isUser && !info->inKernelMode)) {
    // remove mapping
    PTE* currentPTE = searchPTEntryPageDirectory(pd, hostVAddr);
    if (!currentPTE) {
      // already nonexist. Good.
      return true;
    }
    *currentPTE &= ~PE_PRESENT(1);
  } else {
    PTE* directMapPTE = searchPTEntryPageDirectory(info->originalPD,
        guestPAddr + info->baseAddr);
    if (!directMapPTE) {
      // Not a valid guestPAddr
      return false;
    }

    bool shouldBeWritable =
        isWritable || (info->inKernelMode && !info->writeProtection);
    createMapPageDirectory(pd, hostVAddr, PE_DECODE_ADDR(*directMapPTE),
        true, shouldBeWritable);
  }

  if (forceRefresh) {
    invalidateTLB(hostVAddr);
  }
  return true;
}

// Will discard the current mapping
// We don't fear that interrupt may use guest memory: that only happens when
// iret will directly going back.
// Failure may leave page directory in a total mess
// pdGuestAddr must has already applied segment offset
// guestPD must be a kernel space guest PD
static bool reCompileGuestPD(tcb* thr, PageDirectory guestPD) {
  // clear current page table
  clearCurrentPD(thr);

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
      if (!generatePDMapping(thr, isWritable, isUser,
                             RECONSTRUCT_ADDR(i, j), PE_DECODE_ADDR(cpt[j]),
                             false)) {
        return false;
      }
    }
  }

  // Force a context switch to ensure directory change and relavidation
  // (Given the assumtion that a hypervisor only has one thread)
  assert(yieldToNext());
  return true;
}

// shallow - backing up the current pd
static void backupOriginalPD(tcb* thr) {
  HyperInfo* info = &thr->process->hyperInfo;
  PageDirectory pd = thr->process->pd;
  assert(info->originalPD == NULL);
  info->originalPD = newPageDirectory();
  for (int i = STRIP_PD_INDEX(USER_MEM_START); i <= STRIP_PD_INDEX(0xffffffff);
      i++) {
    info->originalPD[i] = pd[i];
  }
}

// Will discard current guest page directory
void reActivateOriginalPD(tcb* thr) {
  HyperInfo* info = &thr->process->hyperInfo;
  PageDirectory pd = thr->process->pd;

  clearCurrentPD(thr);
  for (int i = STRIP_PD_INDEX(USER_MEM_START); i <= STRIP_PD_INDEX(0xffffffff);
      i++) {
    pd[i] = info->originalPD[i];
  }
  // Force a context switch to ensure directory change and relavidation
  // (Given the assumtion that a hypervisor only has one thread)
  assert(yieldToNext());
}

void exitPagingMode(tcb* thr) {
  HyperInfo* info = &thr->process->hyperInfo;
  if (info->originalPD) {
    reActivateOriginalPD(thr);
    sfree(info->originalPD, sizeof(PDE) * PD_SIZE);
  }
}

// NOTE: restore originalPD before calling
bool swtichGuestPD(tcb* thr) {
  HyperInfo* info = &thr->process->hyperInfo;
  uint32_t pdbase = info->vCR3 + info->baseAddr;

  if (!IS_PAGE_ALIGNED(pdbase)) {
    return false;
  }

  // Check the address validation for PD
  if (!verifyUserSpaceAddr(
      pdbase, pdbase + sizeof(PDE) * PD_SIZE - 1, false)) {
    return false;
  }

  PageDirectory guestPD = (PageDirectory)pdbase;
  // Check the address validation for PT
  for (int i = 0; i < PD_SIZE; i++) {
    if (PE_IS_PRESENT(guestPD[i])) {
      uint32_t pteAddrAfterDelta = PE_DECODE_ADDR(guestPD[i]) + info->baseAddr;
      if (!verifyUserSpaceAddr(pteAddrAfterDelta,
          pteAddrAfterDelta + sizeof(PTE) * PT_SIZE - 1, false)) {
        return false;
      }
    }
  }

  // Setup a temporary in-kernel page directory
  PageDirectory tPageDirectory = newPageDirectory();
  for (int i = 0; i < PD_SIZE; i++) {
    if (PE_IS_PRESENT(guestPD[i])) {
      uint32_t pteAddrAfterDelta = PE_DECODE_ADDR(guestPD[i]) + info->baseAddr;
      PageTable tPageTable = clonePageTable((PageTable)pteAddrAfterDelta);
      tPageDirectory[i] =
          PTE_CLEAR_ADDR(guestPD[i]) | PE_DECODE_ADDR((uint32_t)tPageTable);
    }
  }

  bool succ = reCompileGuestPD(thr, tPageDirectory);

  freePageDirectory(tPageDirectory);
  return succ;
}

// NOTE: don't restore originalPD
bool invalidateGuestPDAt(tcb* thr, uint32_t guestaddr) {
  HyperInfo* info = &thr->process->hyperInfo;
  uint32_t pdbase = info->vCR3 + info->baseAddr;

  if (!IS_PAGE_ALIGNED(pdbase)) {
    return false;
  }

  // 1. Make a shallow-copy of current compiled pagetable, before it's cleared
  // Also, we want to protect current page tables from being freed by
  // clearCurrentPD, so we give them all zero
  PageDirectory tPD = newPageDirectory();
  memcpy(tPD, thr->process->pd, sizeof(PDE) * PD_SIZE);
  for (int i = STRIP_PD_INDEX(USER_MEM_START); i <= STRIP_PD_INDEX(0xffffffff);
      i++) {
    thr->process->pd[i] = EMPTY_PDE;
  }

  // 2. activate original pd to fetch the guest pd entry
  reActivateOriginalPD(thr);

  // 3. Check the address validation for PD
  if (!verifyUserSpaceAddr(
      pdbase, pdbase + sizeof(PDE) * PD_SIZE - 1, false)) {
    sfree(tPD, sizeof(PDE) * PD_SIZE);
    return false;
  }

  PageDirectory guestPD = (PageDirectory)pdbase;

  if (PE_IS_PRESENT(guestPD[STRIP_PD_INDEX(guestaddr)])) {
    bool isUser = PE_IS_USERMODE(guestPD[STRIP_PD_INDEX(guestaddr)]);
    bool isWritable = PE_IS_WRITABLE(guestPD[STRIP_PD_INDEX(guestaddr)]);
    PageTable cpt = (PageTable)PE_DECODE_ADDR(
        guestPD[STRIP_PD_INDEX(guestaddr)] + info->baseAddr);
    if (!verifyUserSpaceAddr(
        (uint32_t)cpt, (uint32_t)cpt + sizeof(PTE) * PT_SIZE - 1, false)) {
      sfree(tPD, sizeof(PDE) * PD_SIZE);
      return false;
    }
    if (PE_IS_PRESENT(cpt[STRIP_PT_INDEX(guestaddr)])) {
      isUser &= PE_IS_USERMODE(cpt[STRIP_PT_INDEX(guestaddr)]);
      isWritable &= PE_IS_WRITABLE(cpt[STRIP_PT_INDEX(guestaddr)]);
      uint32_t guestPAddr = PE_DECODE_ADDR(cpt[STRIP_PT_INDEX(guestaddr)]);

      // 4.1. Restore pd copied in step 1 and apply new mapping
      memcpy(thr->process->pd, tPD, sizeof(PDE) * PD_SIZE);
      sfree(tPD, sizeof(PDE) * PD_SIZE);
      assert(yieldToNext());  // force pd flush

      return generatePDMapping(thr, isWritable, isUser, guestaddr,
                               guestPAddr, true);
    }
  }
  // No page directory or page table. Remove it

  // 4.2. Restore pd copied in step 1 and apply new mapping
  memcpy(thr->process->pd, tPD, sizeof(PDE) * PD_SIZE);
  sfree(tPD, sizeof(PDE) * PD_SIZE);
  assert(yieldToNext());  // force pd flush

  return generatePDMapping(thr, false, false, guestaddr, 0xffffffff, true);
}

int hpc_setpd(int userEsp, tcb* thr) {
  DEFINE_PARAM(uint32_t, pdbase, 0);
  DEFINE_PARAM(int, wp, 1);

  HyperInfo* info = &thr->process->hyperInfo;
  if (!info->originalPD) {
    backupOriginalPD(thr);
  }
  reActivateOriginalPD(thr);

  info->vCR3 = pdbase;
  info->writeProtection = (wp == 1);

  if (!swtichGuestPD(thr)) {
    assert(false);
    // TODO Crash it!
  }
  return 0;
}

int hpc_adjustpg(int userEsp, tcb* thr) {
  DEFINE_PARAM(uint32_t, vaddr, 0);

  HyperInfo* info = &thr->process->hyperInfo;
  if (!info->originalPD) {
    // Has not got a valid cr3 yet.
      assert(false);
    // TODO crash it!
  }

  if (!invalidateGuestPDAt(thr, vaddr)) {
    assert(false);
    // TODO Crash it!
  }
  return 0;

}
