/*
 * TODO
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

#define STRIP_PD_INDEX(addr) ((addr) >> (32 - PD_BIT))
#define STRIP_PT_INDEX(addr) \
    (  ((addr) >> (32 - PD_BIT - PT_BIT)) & ((1 << PT_BIT) - 1)  )

#define PE_PRESENT(flag) ((flag) << 0)
#define PE_IS_PRESENT(pe) ((pe) & PE_PRESENT(1))

#define PE_WRITABLE(flag) ((flag) << 1)

#define PE_USERMODE(flag) ((flag) << 2)

#define PE_WRITETHROUGH_CACHE(flag) ((flag) << 3)

#define PE_DISABLE_CACHE(flag) ((flag) << 4)

#define PE_SIZE_FLAG(flag) ((flag) << 7)

#define PT_GLOBAL_FLAG(flag) ((flag) << 8)

#define PE_DECODE_ADDR(pt) ((pt) & 0xfffff000)
#define PDE2PT(pde) ((PageTable)PE_DECODE_ADDR(pde))

//
#define IS_PAGE_ALIGNED(addr) (((addr) >> PAGE_SHIFT << PAGE_SHIFT) == addr)


void enablePaging();

PageDirectory newPageDirectory();

void setKernelMapping(PageDirectory pd);

void createMapPageDirectory(PageDirectory pd, uint32_t vaddr, uint32_t paddr,
    bool isUserMem, bool isWritable);

PTE* searchPTEntryPageDirectory(PageDirectory pd, uint32_t vaddr);

void activatePageDirectory(PageDirectory pd);

PageDirectory getActivePageDirectory();

#endif
