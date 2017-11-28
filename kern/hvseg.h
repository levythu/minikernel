/** @file TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef HVSEG_H
#define HVSEG_H

#include <stdlib.h>
#include <x86/seg.h>
#include <stdint.h>

#include "bool.h"

#define GUEST_PHYSICAL_START USER_MEM_START
#define GUEST_PHYSICAL_LIMIT_MINUS_ONE_PAGE (0xfffff000 - GUEST_PHYSICAL_START)

#define SEGSEL_GUEST_CS (SEGSEL_SPARE0 | 3)
#define SEGSEL_GUEST_DS (SEGSEL_SPARE1 | 3)

void setupVirtualSegmentation();

#endif
