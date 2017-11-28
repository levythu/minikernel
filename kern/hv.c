#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <x86/asm.h>

#include "common_kern.h"
#include "bool.h"
#include "hv.h"

void initHypervisor() {
  setupVirtualSegmentation();
}
