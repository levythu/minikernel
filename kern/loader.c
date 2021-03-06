/**
 * The 15-410 kernel project.
 * @name loader.c
 *
 * Functions for the loading
 * of user programs from binary
 * files should be written in
 * this file. The function
 * elf_load_helper() is provided
 * for your use.
 */

#include <simics.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <exec2obj.h>
#include <loader.h>
#include <elf_410.h>
#include <assert.h>

#include "dbgconf.h"
#include "vm.h"
#include "pm.h"
#include "hv.h"

// Only for debug
void printArgPackage(ArgPackage* pkg) {
  lprintf("++ Print Argument Package:");
  for (int i = 0; i < ARGPKG_MAX_ARG_COUNT; i++) {
    if (pkg->c[i][0] == 0 && pkg->c[i][1] == -1) break;
    lprintf("- %s", pkg->c[i]);
  }
  lprintf("++ End");
}

// Get argc from ArgPackage. Just trace the terminator
static int getArgCount(ArgPackage* pkg) {
  for (int i = 0; i < ARGPKG_MAX_ARG_COUNT; i++) {
    if (pkg->c[i][0] == 0 && pkg->c[i][1] == -1) return i;
  }
  panic("getArgCount: ArgPackage is not valid (without terminator)");
  return -1;
}

/*****************************************************************************/

// Only for debug
void printELF(simple_elf_t* elfMetadata) {
  lprintf("ELF info for %s:", elfMetadata->e_fname);
  lprintf("- Entry Point: 0x%08lx", elfMetadata->e_entry);
  lprintf("- .text: [0x%08lx, 0x%08lx)",
      elfMetadata->e_txtstart, elfMetadata->e_txtstart + elfMetadata->e_txtlen);
  lprintf("- .data: [0x%08lx, 0x%08lx)",
      elfMetadata->e_datstart, elfMetadata->e_datstart + elfMetadata->e_datlen);
  lprintf("- .rodata: [0x%08lx, 0x%08lx)",
      elfMetadata->e_rodatstart,
      elfMetadata->e_rodatstart + elfMetadata->e_rodatlen);
  lprintf("- .bss: [0x%08lx, 0x%08lx)",
      elfMetadata->e_bssstart,
      elfMetadata->e_bssstart + elfMetadata->e_bsslen);
}

// Copy sth to the address in memory [start, end), it's okay that end is 0.
// Will claim new PM->VM mappings (or lift write privilege) if necessary
static int cloneMemoryWithPTERange(PageDirectory pd, uint32_t startAddr,
    uint32_t endAddr, uint32_t sourceStartAddr, bool isWritable) {
  endAddr -= 1;
  if (startAddr > endAddr) return 0;
  uint32_t startPageAddr = PE_DECODE_ADDR(startAddr);
  uint32_t endPageAddr = PE_DECODE_ADDR(endAddr);
  for (uint32_t i = startPageAddr; ; i+=PAGE_SIZE) {
    PTE* pte = searchPTEntryPageDirectory(pd, i);
    if (!pte) {
      // The target page does not exist. Find one available page and create
      // PTE. Also initialize it to all zeros
      uint32_t newPA = getUserMemPage();
      if (!newPA) {
        // Out of memory! Keep the page table as it is, and return error;
        return -1;
      }
      createMapPageDirectory(pd, i, newPA, true, isWritable);
      pte = searchPTEntryPageDirectory(pd, i);
      assert(pte != NULL);
    }

    *pte |= PE_WRITABLE(isWritable);

    // The memory range to set in current page is [pgStart, pgEnd]
    uint32_t pgStart = i;
    if (pgStart < startAddr) pgStart = startAddr;
    uint32_t pgEnd = i - 1 + PAGE_SIZE;
    if (pgEnd > endAddr) pgEnd = endAddr;
    assert(pgStart <= pgEnd);

    // Temporary allow write
    PTE savedPTE = *pte;
    *pte |= PE_WRITABLE(1);
    invalidateTLB(i);

    if (sourceStartAddr != 0) {
      // copy contents from source memory
      memcpy((void*)pgStart,
             (void*)(pgStart - startAddr + sourceStartAddr),
             pgEnd - pgStart + 1);
    } else {
      // set them all to zero
      memset((void*)pgStart,
             0,
             pgEnd - pgStart + 1);
    }
    *pte = savedPTE;
    invalidateTLB(i);

    // We must check here instead of for-loop header, for overflow concerns
    if (i == endPageAddr) break;
  }

  return 0;
}

// Set up the initial stack for a new elf so that
// _main fucntion in crt0.c will have correct view of parameters.
// argpkg define the argv and caller should dispose it, it can also be NULL,
// and (argc=0, argv={NULL}) will be initiated
static void setupInitialStack(ArgPackage* argpkg,
    ProcessMemoryMeta* memMeta, uint32_t *esp) {
  *esp = memMeta->stackHigh - sizeof(void*) + 1;
  int argc;

  if (argpkg) {
    // push ArgPackage on stack!
    *esp -= sizeof(ArgPackage);
    memcpy((void*)*esp, (void*)argpkg, sizeof(ArgPackage));
    ArgPackage* pkgOnStack = (ArgPackage*)(*esp);
    argc = getArgCount(pkgOnStack);
    // push argv table on stack
    for (int i = argc; i >= 0; i--) {
      uint32_t strAddr;
      if (i == argc) {
        strAddr = 0;  // NULL terminator
      } else {
        strAddr = (uint32_t)(pkgOnStack->c[i]);
      }
      *esp -= sizeof(uint32_t);
      *(uint32_t*)(*esp) = strAddr;
    }
  } else {
    // Push a null terminated thing
    argc = 0;
    *esp -= sizeof(uint32_t);
    *(uint32_t*)(*esp) = 0;
  }

  uint32_t* initialStack = (uint32_t*)(*esp);
  initialStack[-1] = memMeta->stackLow;   // stacklow
  initialStack[-2] = memMeta->stackHigh;   // stackhigh
  initialStack[-3] = *esp;   // argv
  initialStack[-4] = argc;     // argc
  initialStack[-5] = 0xdeadbeef;  // return address: invalid one
  *esp = (uint32_t)&initialStack[-5];
}

// see loader.h
int initELFMemory(const char *filename, PageDirectory pd, ArgPackage* argpkg,
    ProcessMemoryMeta* memMeta, uint32_t* eip, uint32_t *esp, HyperInfo* info) {
  #ifdef VERBOSE_PRINT
    lprintf("Loading %s", filename);
  #endif
  if (elf_check_header(filename) != ELF_SUCCESS) {
    lprintf("%s is not a valid elf", filename);
    return -1;
  }
  simple_elf_t elfMetadata;
  if (elf_load_helper(&elfMetadata, filename) != ELF_SUCCESS) {
    lprintf("%s fail to parse elf", filename);
    return -1;
  }

  if (fillHyperInfo(&elfMetadata, info)) {
    lprintf("bootstrapping virtual machine...");
    // Allocate the whole virtual memory
    if (cloneMemoryWithPTERange(pd, GUEST_PHYSICAL_START,
                                GUEST_PHYSICAL_START + HYPERVISOR_MEMORY,
                                0,
                                true) < 0) {
      return -1;
    }
    #ifdef VERBOSE_PRINT
      lprintf("Allocated bootstrapping memory for vm");
    #endif
  }

  #ifdef VERBOSE_PRINT
    printELF(&elfMetadata);
  #endif

  char* fileContentTmp;

  // Init .text
  if (elfMetadata.e_txtlen > 0) {
    fileContentTmp = smalloc(elfMetadata.e_txtlen);
    assert(
      getbytes(filename, elfMetadata.e_txtoff,
          elfMetadata.e_txtlen, fileContentTmp) == elfMetadata.e_txtlen
    );
    if (cloneMemoryWithPTERange(pd, elfMetadata.e_txtstart,
                                elfMetadata.e_txtstart + elfMetadata.e_txtlen,
                                (uint32_t)fileContentTmp,
                                false) < 0) {
      sfree(fileContentTmp, elfMetadata.e_txtlen);
      return -1;
    }
    sfree(fileContentTmp, elfMetadata.e_txtlen);
  }
  #ifdef VERBOSE_PRINT
    lprintf("Inited text segment");
  #endif

  // Init .data
  if (elfMetadata.e_datlen > 0) {
    fileContentTmp = smalloc(elfMetadata.e_datlen);
    assert(
      getbytes(filename, elfMetadata.e_datoff,
          elfMetadata.e_datlen, fileContentTmp) == elfMetadata.e_datlen
    );
    if (cloneMemoryWithPTERange(pd, elfMetadata.e_datstart,
                                elfMetadata.e_datstart + elfMetadata.e_datlen,
                                (uint32_t)fileContentTmp,
                                true) < 0) {
      sfree(fileContentTmp, elfMetadata.e_datlen);
      return -1;
    }
    sfree(fileContentTmp, elfMetadata.e_datlen);
    #ifdef VERBOSE_PRINT
      lprintf("Inited data segment");
    #endif
  }

  // Init .rodata
  if (elfMetadata.e_rodatlen > 0) {
    fileContentTmp = smalloc(elfMetadata.e_rodatlen);
    assert(
      getbytes(filename, elfMetadata.e_rodatoff,
          elfMetadata.e_rodatlen, fileContentTmp) == elfMetadata.e_rodatlen
    );
    if (cloneMemoryWithPTERange(pd, elfMetadata.e_rodatstart,
                                elfMetadata.e_rodatstart + elfMetadata.e_rodatlen,
                                (uint32_t)fileContentTmp,
                                false) < 0) {
      sfree(fileContentTmp, elfMetadata.e_rodatlen);
      return -1;
    }
    sfree(fileContentTmp, elfMetadata.e_rodatlen);
    #ifdef VERBOSE_PRINT
      lprintf("Inited readonly data segment");
    #endif
  }

  // Init .bss
  if (elfMetadata.e_bssstart > 0) {
    if (cloneMemoryWithPTERange(pd, elfMetadata.e_bssstart,
                                elfMetadata.e_bssstart + elfMetadata.e_bsslen,
                                0,
                                true) < 0) {
      return -1;
    }
    #ifdef VERBOSE_PRINT
      lprintf("Inited bss segment");
    #endif
  }

  if (!info->isHyper) {
    // Init stack, give it two page
    // the bottom one will hold ArgPackage
    // Since it gets written immediately, we don't use ZFOD here
    if (cloneMemoryWithPTERange(pd, 0 - (uint32_t)(PAGE_SIZE * 2),
                                0,    // 0xffffffff + 1
                                0,
                                true) < 0) {
      return -1;
    }
    #ifdef VERBOSE_PRINT
      lprintf("Allocated stack");
    #endif

    // Makeup mem meta
    memMeta->stackHigh = 0xffffffff;
    memMeta->stackLow = 0 - (uint32_t)(PAGE_SIZE * 2);
    // No fixed heap place in pebble syscall, so leave it.
    memMeta->heapLow = 0;
    memMeta->heapSize = 0;
    setupInitialStack(argpkg, memMeta, esp);
    #ifdef VERBOSE_PRINT
      lprintf("Inited stack");
    #endif
  }
  *eip = elfMetadata.e_entry;

  return 0;
}

/**
 * Copies data from a file into a buffer.
 *
 * @param filename   the name of the file to copy data from
 * @param offset     the location in the file to begin copying from
 * @param size       the number of bytes to be copied
 * @param buf        the buffer to copy the data into
 *
 * @return returns the number of bytes copied on succes; -1 on failure
 */
int getbytes(const char *filename, int offset, int size, char *buf) {
  for (int i = 0; i < exec2obj_userapp_count; i++) {
    if (strcmp(exec2obj_userapp_TOC[i].execname, filename) != 0) continue;
    if (exec2obj_userapp_TOC[i].execlen - offset < size) {
      size = exec2obj_userapp_TOC[i].execlen - offset;
    }
    if (size < 0) return -1;
    if (size == 0) return 0;
    memcpy(buf, &exec2obj_userapp_TOC[i].execbytes[offset], size);
    return size;
  }
  // File does not exist
  return -1;
}
