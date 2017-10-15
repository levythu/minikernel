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

#include "dbgconf.h"

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

int loadFile(const char *filename) {
  if (elf_check_header(filename) != ELF_SUCCESS) {
    lprintf("%s is not a valid elf", filename);
    return -1;
  }
  simple_elf_t elfMetadata;
  if (elf_load_helper(&elfMetadata, filename) != ELF_SUCCESS) {
    lprintf("%s fail to parse elf", filename);
    return -1;
  }

  #ifdef VERBOSE_PRINT
    printELF(&elfMetadata);
  #endif

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
    if (size <= 0) return 0;
    memcpy(buf, &exec2obj_userapp_TOC[i].execbytes[offset], size);
    return size;
  }
  // File does not exist
  return -1;
}
