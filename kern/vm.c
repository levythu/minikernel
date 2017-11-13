/** @file vm.c
 *
 *  @brief Functions and utilities for virtual memory
 *
 *  All that are needed for manipulating virtual memory and page table
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "dbgconf.h"
#include "x86/asm.h"
#include "x86/cr.h"
#include "common_kern.h"
#include "vm.h"
#include "bool.h"

PageDirectory newPageDirectory() {
  PageDirectory newPD = (PDE*)smemalign(PAGE_SIZE, sizeof(PDE) * PD_SIZE);
  if (!newPD) {
    panic("newPageDirectory: fail to allocate space for new page directory");
  }
  for (int i = 0; i < PD_SIZE; i++) {
    newPD[i] = PE_PRESENT(0) | PE_WRITABLE(0) | PE_USERMODE(0) |
               PE_WRITETHROUGH_CACHE(0) | PE_DISABLE_CACHE(0) |
               PE_SIZE_FLAG(0);
  }
  return newPD;
}

void freePageDirectory(PageDirectory pd) {
  for (int i = 0; i < PD_SIZE; i++) {
    if (PE_IS_PRESENT(pd[i])) {
      sfree(PDE2PT(pd[i]), sizeof(PTE) * PT_SIZE);
    }
  }
  sfree((void*)pd, sizeof(PDE) * PD_SIZE);
}

static PageTable newPageTable() {
  PageTable newPT = (PTE*)smemalign(PAGE_SIZE, sizeof(PTE) * PT_SIZE);
  if (!newPT) {
    panic("newPageTable: fail to allocate space for new page table");
  }
  for (int i = 0; i < PT_SIZE; i++) {
    newPT[i] = PE_PRESENT(0) | PE_WRITABLE(0) | PE_USERMODE(0) |
               PE_WRITETHROUGH_CACHE(0) | PE_DISABLE_CACHE(0) |
               PE_SIZE_FLAG(0) | PT_GLOBAL_FLAG(0);
  }
  return newPT;
}

void createMapPageDirectory(PageDirectory pd, uint32_t vaddr, uint32_t paddr,
    bool isUserMem, bool isWritable) {
  assert(PE_DECODE_ADDR(vaddr) == vaddr);
  assert(PE_DECODE_ADDR(paddr) == paddr);
  // #ifdef VERBOSE_PRINT
  //   if (isUserMem) lprintf("Mapping 0x%08lx to 0x%08lx", vaddr, paddr);
  // #endif

  uint32_t pdIndex = STRIP_PD_INDEX(vaddr);
  uint32_t ptIndex = STRIP_PT_INDEX(vaddr);
  if (!PE_IS_PRESENT(pd[pdIndex])) {
    // Create new page table, use the indicated privilege level
    // Always use writable privilege at page directory level, and specify
    // write permission in page table level.
    PageTable newPT = newPageTable();
    assert(PE_DECODE_ADDR((uint32_t)newPT) ==  (uint32_t)newPT);

    pd[pdIndex] = PE_PRESENT(1) | PE_WRITABLE(1) | PE_USERMODE(isUserMem) |
                  PE_WRITETHROUGH_CACHE(0) | PE_DISABLE_CACHE(0) |
                  PE_SIZE_FLAG(0) | PT_GLOBAL_FLAG(0) | (uint32_t)newPT;
  }
  PDE2PT(pd[pdIndex])[ptIndex] =
                         PE_PRESENT(1) |
                         PE_WRITABLE(isWritable) | PE_USERMODE(isUserMem) |
                         PE_WRITETHROUGH_CACHE(0) | PE_DISABLE_CACHE(0) |
                         PE_SIZE_FLAG(0) | PT_GLOBAL_FLAG(0) | paddr;
}

PageTable clonePageTable(PageTable old) {
  PageTable newPT = newPageTable();
  memcpy((void*)newPT, (void*)old, sizeof(PTE) * PT_SIZE);
  return newPT;
}

// [startPDIndex, endPDIndex]
// The PD index must be nonexist
void clonePageDirectory(PageDirectory src, PageDirectory dst,
    uint32_t startPDIndex, uint32_t endPDIndex) {
  for (int i = startPDIndex; i <= endPDIndex; i++) {
    if (PE_IS_PRESENT(dst[i])) {
      panic("clonePageDirectory: cannot clone to a dest pd with "
            "conflicting page table");
    }
    dst[i] = src[i];
    if (PE_IS_PRESENT(src[i])) {
      PageTable newPT = clonePageTable(PDE2PT(src[i]));
      dst[i] = PDE_CLEAR_PT(src[i]) | (uint32_t)newPT;
    }
  }
}

uint32_t traverseEntryPageDirectory(PageDirectory pd,
    uint32_t startPDIndex, uint32_t endPDIndex,
    uint32_t (*onPTE)(int, int, PTE*, uint32_t),
    uint32_t initialToken) {
  for (int i = startPDIndex; i <= endPDIndex; i++) {
    if (!PE_IS_PRESENT(pd[i])) continue;
    PageTable cpt = PDE2PT(pd[i]);
    for (int j = 0; j < PT_SIZE; j++) {
      if (!PE_IS_PRESENT(cpt[j])) continue;
      initialToken = onPTE(i, j, &cpt[j], initialToken);
    }
  }
  return initialToken;
}

PTE* searchPTEntryPageDirectory(PageDirectory pd, uint32_t vaddr) {
  assert(PE_DECODE_ADDR(vaddr) == vaddr);

  uint32_t pdIndex = STRIP_PD_INDEX(vaddr);
  uint32_t ptIndex = STRIP_PT_INDEX(vaddr);

  if (!PE_IS_PRESENT(pd[pdIndex])) return NULL;
  if (!PE_IS_PRESENT(PDE2PT(pd[pdIndex])[ptIndex])) return NULL;
  return &PDE2PT(pd[pdIndex])[ptIndex];
}

void activatePageDirectory(PageDirectory pd) {
  uint32_t pdNum = (uint32_t)pd;
  assert(PE_DECODE_ADDR(pdNum) == pdNum);
  set_cr3(pdNum);
}

PageDirectory getActivePageDirectory() {
  return (PageDirectory)PE_DECODE_ADDR(get_cr3());
}

void setKernelMapping(PageDirectory pd) {
  for (int i = 0; i < USER_MEM_START; i+= PAGE_SIZE) {
    createMapPageDirectory(pd, i, i, false, true);
  }
}

static PageDirectory initPD;
void enablePaging() {
  // Set up a table with direct map only on kernel addresses
  initPD = newPageDirectory();
  setKernelMapping(initPD);
  activatePageDirectory(initPD);
  set_cr0(get_cr0() | CR0_PG | CR0_WP);
  lprintf("Initial page directory established.");
}
