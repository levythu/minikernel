/* The 15-410 kernel project
 *
 *     loader.h
 *
 * Structure definitions, #defines, and function prototypes
 * for the user process loader.
 */

#ifndef _LOADER_H
#define _LOADER_H

#include <stdint.h>

#include "bool.h"
#include "vm.h"

#define ARGPKG_MAX_ARG_LEN 128
#define ARGPKG_MAX_ARG_COUNT 32

// Sum up to 4096, and it contains the terminating zero, both in string and
// array.
// every entry is a string terminated by zero. The space after the entry is
// also zero padding
// But the terminator of arg is "\0\0xff"
// i.e., ArgPackage can only holds (ARGPKG_MAX_ARG_COUNT-1) real strings plus
// a special terminator
typedef struct {
  char c[ARGPKG_MAX_ARG_COUNT][ARGPKG_MAX_ARG_LEN];
} ArgPackage;

void printArgPackage(ArgPackage* pkg);

/*****************************************************************************/

// This is the memory situation after an elf is loaded.
// To be more concise, it describe the range of stack and range of heap
// [stackLow, stackHigh] is the stack range
// [heapLow, heapLow + heapSize) is the heap range (may be zero size)
typedef struct {
  uint32_t stackHigh;
  uint32_t stackLow;
  uint32_t heapLow;
  uint32_t heapSize;
} ProcessMemoryMeta;

/* --- Prototypes --- */
// Get bytes from filename [offset, offset + size) to buf.
// Return the number of bytes in buf.
// Or -1 if there's any failure
int getbytes( const char *filename, int offset, int size, char *buf );

// Load the ELF into memory, allocate virtual memory if needed. After the
// the function, the program is ready to run from
// `_main(int argc, char *argv[], void *stack_high, void *stack_low)`
// at crt0.c with all parameters set.
//
// - filename: the file to load
// - pd: current page directory (the caller must ensure the pd is activated
//       otherwise the memory is not accessible)
// - memMeta: if success, memMeta will contain the info for stack and heap
// - eip: if success, eip will be the entry of the program in memory
// - esp: if success, esp is the current top of stack (with initial parameter
//       for _main set)
// Return: 0 if success, -1 otherwise
int initELFMemory(const char *filename, PageDirectory pd,
     ProcessMemoryMeta* memMeta, uint32_t* eip, uint32_t *esp);

#endif /* _LOADER_H */
