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
