/** @file pm.c
 *
 *  @brief TODO
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "x86/asm.h"
#include "x86/cr.h"
#include "x86/eflags.h"
#include "common_kern.h"
#include "pm.h"
#include "vm.h"
#include "bool.h"
#include "cpu.h"
#include "mode_switch.h"

// Only bit 0/2/4/6/7/8/10/11 can be changed in user mode,
// And bit 16 (RF) is auto-set by instr, we ignore it
#define EFLAGS_USER_WRITABLE_BIT 0x10DD5        // 1 0000 1101 1101 0101
static bool validateEFLAGS(uint32_t eflags) {
  uint32_t standardEflags = (get_eflags() | EFL_RESV1 | EFL_IF) & ~EFL_AC;
  eflags ^= standardEflags;
  if (eflags & ~EFLAGS_USER_WRITABLE_BIT) {
    return false;
  }
  return true;
}

bool validateUregs(ureg_t* uregs) {
  if (!validateEFLAGS(uregs->eflags)) return false;
  return true;
}
