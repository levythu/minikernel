/** @file vm.h
 *
 *  @brief Functions and utilities for virtual memory
 *
 *  All that are needed for manipulating virtual memory and page table. See
 *  comments below for each util
 *
 *  @author Leiyu Zhao
 */

#ifndef VM_H
#define VM_H

#include <stdlib.h>
#include <stdint.h>
#include <x86/page.h>

#include "bool.h"

typedef uint32_t PTE;
typedef PTE* PageTable;
typedef uint32_t PDE;
typedef PDE* PageDirectory;

// The Page-table/dir entry structure
#define PD_BIT 10
#define PT_BIT 10
#define PD_SIZE (1 << PD_BIT)
#define PT_SIZE (1 << PT_BIT)

// Given a virtual memory address, extract the page directory index
#define STRIP_PD_INDEX(addr) ((addr) >> (32 - PD_BIT))

// Given a virtual memory address, extract the page table index inside some page
// directory
#define STRIP_PT_INDEX(addr) \
    (  ((addr) >> (32 - PD_BIT - PT_BIT)) & ((1 << PT_BIT) - 1)  )

// Given a page directory and page table index, reconstruct the virtual memory
#define RECONSTRUCT_ADDR(pdindex, ptindex) \
    (  ((pdindex) << (32 - PD_BIT)) | ((ptindex) << (32 - PD_BIT - PT_BIT))  )

#define PE_PRESENT(flag) ((flag) << 0)
#define PE_IS_PRESENT(pe) ((pe) & PE_PRESENT(1))

// Page Directory is always writable
#define PE_WRITABLE(flag) ((flag) << 1)
#define PE_IS_WRITABLE(pe) ((pe) & PE_WRITABLE(1))

#define PE_USERMODE(flag) ((flag) << 2)
#define PE_IS_USERMODE(pe) ((pe) & PE_USERMODE(1))

#define PE_WRITETHROUGH_CACHE(flag) ((flag) << 3)

#define PE_DISABLE_CACHE(flag) ((flag) << 4)

#define PE_SIZE_FLAG(flag) ((flag) << 7)

#define PT_GLOBAL_FLAG(flag) ((flag) << 8)

#define PE_ENCODE_CUSTOM(threebit) ((threebit) << 9)
#define PE_DECODE_CUSTOM(pe) (((pe) >> 9) & 7)

#define PE_DECODE_ADDR(pt) ((pt) & 0xfffff000)
#define PDE2PT(pde) ((PageTable)PE_DECODE_ADDR(pde))
#define PDE_CLEAR_PT(pde) ((pde) & 0xfff)
#define PTE_CLEAR_ADDR(pte) PDE_CLEAR_PT(pte)

#define IS_PAGE_ALIGNED(addr) (((addr) >> PAGE_SHIFT << PAGE_SHIFT) == addr)

#define EMPTY_PDE (PE_PRESENT(0) | PE_WRITABLE(0) | PE_USERMODE(0) | \
                   PE_WRITETHROUGH_CACHE(0) | PE_DISABLE_CACHE(0) | \
                   PE_SIZE_FLAG(0))

// Start paging (vm)
void enablePaging();

// create a page directory, with nothing mapped
PageDirectory newPageDirectory();

// Only free the page directory (and recursively page table), not any physical
// page associated.
void freePageDirectory(PageDirectory pd);

// Given a page directory, set initial kernel memory direct mapping
void setKernelMapping(PageDirectory pd);

// Map one physical page to a virtual page in the directory, given the access
// ring and write privilege
void createMapPageDirectory(PageDirectory pd, uint32_t vaddr, uint32_t paddr,
    bool isUserMem, bool isWritable);

// return the reference of the page table entry for one virtual address
// Or NULL if it's not presented in the page directory
PTE* searchPTEntryPageDirectory(PageDirectory pd, uint32_t vaddr);

// Adopt the page directory
void activatePageDirectory(PageDirectory pd);

// Get what page directory is adopted
PageDirectory getActivePageDirectory();

// Clone a page table, associated physical pages are not cloned
PageTable clonePageTable(PageTable old);

// Recursively clone a page directory (and page table), from startPDIndex
// to endPDIndex. associated physical pages are not cloned
// dst must not present page table in the range [startPDIndex, endPDIndex]
void clonePageDirectory(PageDirectory src, PageDirectory dst,
    uint32_t startPDIndex, uint32_t endPDIndex);

// Traverse all presented pages in one page directory, given range
// [startPDIndex, endPDIndex]. initialToken is a user-given token that can be
// passed on each onPTE call, so this function acts as fold() on onPTE
uint32_t traverseEntryPageDirectory(PageDirectory pd,
    uint32_t startPDIndex, uint32_t endPDIndex,
    uint32_t (*onPTE)(int, int, PTE*, uint32_t),
    uint32_t initialToken);

// Invalid the TLB cache for given virtual address
// When modify current page directory without switching the whole PD, this one
// need to called to ensure consistent
void invalidateTLB(uint32_t addr);

#endif
