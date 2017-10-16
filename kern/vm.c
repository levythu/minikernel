/** @file vm.c
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>

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
  #ifdef VERBOSE_PRINT
    if (isUserMem) lprintf("Mapping 0x%08lx to 0x%08lx", vaddr, paddr);
  #endif

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
  set_cr0(get_cr0() | CR0_PG);
  lprintf("Initial page directory established.");
}
