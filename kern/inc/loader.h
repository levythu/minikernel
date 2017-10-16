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

typedef struct {
  uint32_t stackHigh;
  uint32_t stackLow;
  uint32_t heapLow;
  uint32_t heapSize;
} ProcessMemoryMeta;

/* --- Prototypes --- */

int getbytes( const char *filename, int offset, int size, char *buf );

/*
 * Declare your loader prototypes here.
 */

int initELFMemory(const char *filename, PageDirectory pd,
     ProcessMemoryMeta* memMeta, uint32_t* eip, uint32_t *esp);

#endif /* _LOADER_H */
