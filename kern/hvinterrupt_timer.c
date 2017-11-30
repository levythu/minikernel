#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <hvcall_int.h>
#include <hvcall.h>
#include <x86/asm.h>
#include <x86/timer_defines.h>

#include "int_handler.h"
#include "common_kern.h"
#include "bool.h"
#include "process.h"
#include "cpu.h"
#include "hv.h"
#include "hvlife.h"
#include "source_untrusted.h"
#include "hv_hpcall_internal.h"
#include "hvinterrupt.h"
#include "hvinterrupt_timer.h"
#include "zeus.h"

intMultiplexer timeMultiplexter;

void hv_CallMeOnTick() {
  hvInt tint;
  tint.intNum = TIMER_IDT_ENTRY;
  broadcastIntTo(&timeMultiplexter, tint);
}