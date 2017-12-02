/** @file hv.c
 *
 *  @brief Implementation of basic hypervisor initiator
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <x86/asm.h>

#include "common_kern.h"
#include "bool.h"
#include "hv.h"
#include "hvinterrupt_pushevent.h"

void initHypervisor() {
  setupVirtualSegmentation();
  initHyperCall();
}
