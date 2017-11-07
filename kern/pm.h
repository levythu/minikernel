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

uint32_t getUserMemPageZFOD();

// Upgrade never fail
uint32_t upgradeUserMemPageZFOD(uint32_t mem);

bool isZFOD(uint32_t addr);

// Free one user page that is previously got by calling getUserMemPage or
// getUserMemPageZFOD. After that you cannot use the memory anymore
void freeUserMemPage(uint32_t mem);

void reportUserMem();

#endif
