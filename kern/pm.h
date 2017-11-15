/** @file pm.h
 *
 *  @brief Physical Memory manager, only for non-kernel space.
 *
 *  This module manages all non-kernel physical memory. And provide them to
 *  kernel to set vm-to-pm mappings
 *
 *  All functions except claimUserMem are interrupt-safe and multicore-safe.
 *
 *  @author Leiyu Zhao
 */

#ifndef PM_H
#define PM_H

#include <stdlib.h>
#include <x86/page.h>

#include "bool.h"

// Init code, should be called in bootstrap
void claimUserMem();

// Get one 4k-aligned user page, will return the start address
// or 0 if there's no free space available
uint32_t getUserMemPage();

// similar to getUserMemPage, but the caller should know that this is a fake
// shared page with all contents equal to zero. Caller should register this as
// a readonly page, and ask for upgrade when anyone wants to write it.
uint32_t getUserMemPageZFOD();

// Given a physical page that's returned by getUserMemPageZFOD(), return a
// real page that's dedicated and all-zero'd. Caller should replace the old page
// with the new one in page table.
// Upgrade never fail.
uint32_t upgradeUserMemPageZFOD(uint32_t mem);

// return true if the current phyisical address is ZFOD'd that's not upgraded
bool isZFOD(uint32_t addr);

// Free one user page that is previously got by calling getUserMemPage or
// getUserMemPageZFOD. After that you cannot use the page anymore
void freeUserMemPage(uint32_t mem);

// Report user space usage, use this to detect memory leak
void reportUserMem();

#endif
