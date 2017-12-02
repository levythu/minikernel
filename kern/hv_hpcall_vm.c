/** @file hv_hpcall_vm.h
 *
 *  @brief All about guest virtual memory and related hypercalls
 *
 *  In hyperInfo, there're several fields about guest virtual memory
 *  (see hvlife.h). And they work in the following way:
 *
 *  When paging is off, hypervisor process keeps default direct mapping page
 *  directory (original page directory). And originalPD = null.
 *
 *  As long as paging is turned on, the original mapping is shallow copied (
 *  copy page directory only, not page tables) to originalPD. originalPD is used
 *  in the following situation:
 *  - When we need to access the guest physical memory (e.g. access guest PD)
 *  - When we are about to exit, so that we must recover original page directory
 *    to let Reaper reclaim all user-pages (see reaper.c)
 *
 *  We don't keep a copy of guest page table in kernel memory, instead, we
 *  recompile it everytime on 1. change cr3; 2. mode switch.
 *
 *  Except adjustpg, which just changes a one mapping, every large-scale mapping
 *  mutation (e.g. change cr3, mode switch, temporarily use originalPD) needs
 *  flush the whole TLB. To avoid any race with context switcher itself, we
 *  deligate it to context switch, i.e., achieving flushing TLB by forcing a
 *  context switch to other threads. Since for hypervisor there should only be
 *  one thread (in host kernel's view), there's no race at all.
 *
 *  @author Leiyu Zhao
 */

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


// Clear the current page directory's user space
// If it's the same as backed pd, then it's just a shallow clear (remove them
// from page directory)
// Otherwise it's a deep clear (free related page tables)
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

// The key function for establishing a mapping from guest virtual addr to guest
// physical addr in host's page directory.
// guestPAddr = 0xffffffff for remove
// return true for success
static bool generatePDMapping(tcb* thr, bool isWritable, bool isUser,
    uint32_t guestVAddr, uint32_t guestPAddr, bool forceRefresh) {
  HyperInfo* info = &thr->process->hyperInfo;
  PageDirectory pd = thr->process->pd;

  // Validate guestVAddr and guestPAddr
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
    // create mapping

    // 1. Find the host physical address of given guest physical address
    PTE* directMapPTE = searchPTEntryPageDirectory(info->originalPD,
        guestPAddr + info->baseAddr);
    if (!directMapPTE) {
      // Not a valid guestPAddr
      return false;
    }

    // 2. Make the mapping in page directory
    // The page is writable if: 1. it's defined as writable; 2. guest is in
    // ring0 and write protection is off.
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

// Check if a PTE/PDE is a "good" one, iff it uses no bits other than:
// 1. present bit (bit0)
// 2. writable bit (bit1)
// 3. privilege bit (bit2)
// 4. user-custom bits (bits 9~11)
// 5. actual memory bits (bits >=12)
#define IS_VALID_GUEST_PE(pe) \
    ( \
      ( \
        PTE_CLEAR_ADDR(pe) & \
        ~(PE_PRESENT(1) | PE_WRITABLE(1) |  \
          PE_USERMODE(1) | PE_ENCODE_CUSTOM(3)) \
      ) == 0 \
    )

// Recompile given guestPD to current page directory and activate it
// Will discard the current mapping
// We don't fear that interrupt may use guest memory: that only happens when
// iret will directly going back.
// Failure may leave page directory (user part) in a total mess, so caller need
// to follow standard crashing sequence (reActivate originalPD and vanish)
// guestPD must be a kernel space guest PD
static bool reCompileGuestPD(tcb* thr, PageDirectory guestPD) {
  // clear current page table
  clearCurrentPD(thr);

  // compile
  for (int i = 0; i < PD_SIZE; i++) {
    if (!PE_IS_PRESENT(guestPD[i])) continue;
    if (!IS_VALID_GUEST_PE(guestPD[i])) {
      return false;
    }
    bool isUserRoot = PE_IS_USERMODE(guestPD[i]);
    bool isWritableRoot = PE_IS_WRITABLE(guestPD[i]);
    PageTable cpt = PDE2PT(guestPD[i]);
    for (int j = 0; j < PT_SIZE; j++) {
      if (!PE_IS_PRESENT(cpt[j])) continue;
      bool isUser = isUserRoot && PE_IS_USERMODE(cpt[j]);
      bool isWritable = isWritableRoot && PE_IS_WRITABLE(cpt[j]);
      if (!IS_VALID_GUEST_PE(cpt[j])) {
        return false;
      }
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

// shallow - backing up the current pd to originalPD. This cannot be called more
// than once: just do it when first turning on paging
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

// Discard current guest page directory and activate original PD.
// This is needed before crashing or want to accessing guest page directory
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

// recover originalPD, free its back up and permannently leave paging mode
// Hypercall is not allowed to turn off paging, so this only happens on
// hypervisor exiting
void exitPagingMode(tcb* thr) {
  HyperInfo* info = &thr->process->hyperInfo;
  if (info->originalPD) {
    reActivateOriginalPD(thr);
    sfree(info->originalPD, sizeof(PDE) * PD_SIZE);
  }
}

// The wrapper of reCompileGuestPD, since reCompileGuestPD needs a in-memory
// copy of guest page directory. This function make a temporary copy, call it,
// and destroy the copy.
// NOTE: this function will access guest physical memory, so caller must
// recover originalPD before calling me
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

// Invalidate only one mapping is complicated: we have to clear the current
// page directory, recover originalPD; while we need to preserve most.
// Simply recover originalPD will cause loss to current PD. Therefore, caller
// mustn't recover originalPD before calling this. (it's different from
// swtichGuestPD)
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
    if (!IS_VALID_GUEST_PE(guestPD[STRIP_PD_INDEX(guestaddr)])) {
      sfree(tPD, sizeof(PDE) * PD_SIZE);
      return false;
    }
    PageTable cpt = (PageTable)PE_DECODE_ADDR(
        guestPD[STRIP_PD_INDEX(guestaddr)] + info->baseAddr);
    if (!verifyUserSpaceAddr(
        (uint32_t)cpt, (uint32_t)cpt + sizeof(PTE) * PT_SIZE - 1, false)) {
      sfree(tPD, sizeof(PDE) * PD_SIZE);
      return false;
    }
    if (PE_IS_PRESENT(cpt[STRIP_PT_INDEX(guestaddr)])) {
      if (!IS_VALID_GUEST_PE(cpt[STRIP_PT_INDEX(guestaddr)])) {
        sfree(tPD, sizeof(PDE) * PD_SIZE);
        return false;
      }
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

// Below are simple entries for virtual memory -related hypercalls

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
    lprintf("Hypervisor crashes: fail to set new page directory");
    exitHyperWithStatus(info, thr, GUEST_CRASH_STATUS);
  }
  return 0;
}

int hpc_adjustpg(int userEsp, tcb* thr) {
  DEFINE_PARAM(uint32_t, vaddr, 0);

  HyperInfo* info = &thr->process->hyperInfo;
  if (!info->originalPD) {
    // Has not got a valid cr3 yet.
    lprintf("Hypervisor crashes: cannot call adjustpg before setting cr3");
    exitHyperWithStatus(info, thr, GUEST_CRASH_STATUS);
  }

  if (!invalidateGuestPDAt(thr, vaddr)) {
    lprintf("Hypervisor crashes: fail to adjust page");
    exitHyperWithStatus(info, thr, GUEST_CRASH_STATUS);
  }
  return 0;

}
