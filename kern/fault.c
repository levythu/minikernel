/** @file fault.c
 *
 *  @brief TODO
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <x86/asm.h>
#include <x86/cr.h>
#include <x86/idt.h>

#include "common_kern.h"
#include "pm.h"
#include "vm.h"
#include "bool.h"
#include "cpu.h"
#include "fault_handler_internal.h"
#include "int_handler.h"

DECLARE_FAULT_ENTRANCE(IDT_DE);  // SWEXN_CAUSE_DIVIDE
DECLARE_FAULT_ENTRANCE(IDT_DB);  // SWEXN_CAUSE_DEBUG
DECLARE_FAULT_ENTRANCE(IDT_NMI); // Non-interrupted exception
DECLARE_FAULT_ENTRANCE(IDT_BP);  // SWEXN_CAUSE_BREAKPOINT
DECLARE_FAULT_ENTRANCE(IDT_OF);  // SWEXN_CAUSE_OVERFLOW
DECLARE_FAULT_ENTRANCE(IDT_BR);  // SWEXN_CAUSE_BOUNDCHECK
DECLARE_FAULT_ENTRANCE(IDT_UD);  // SWEXN_CAUSE_OPCODE
DECLARE_FAULT_ENTRANCE(IDT_NM);  // SWEXN_CAUSE_NOFPU
DECLARE_FAULT_ENTRANCE(IDT_CSO); // ??
DECLARE_FAULT_ENTRANCE(IDT_NP);  // SWEXN_CAUSE_SEGFAULT
DECLARE_FAULT_ENTRANCE(IDT_SS);  // SWEXN_CAUSE_STACKFAULT
DECLARE_FAULT_ENTRANCE(IDT_GP);  // SWEXN_CAUSE_PROTFAULT
DECLARE_FAULT_ENTRANCE(IDT_PF);  // SWEXN_CAUSE_PAGEFAULT
DECLARE_FAULT_ENTRANCE(IDT_MF);  // SWEXN_CAUSE_FPUFAULT
DECLARE_FAULT_ENTRANCE(IDT_AC);  // SWEXN_CAUSE_ALIGNFAULT
DECLARE_FAULT_ENTRANCE(IDT_XF);  // SWEXN_CAUSE_SIMDFAULT

void registerFaultHandler() {
  MAKE_FAULT_IDT(IDT_DE);
  MAKE_FAULT_IDT(IDT_DB);
  MAKE_FAULT_IDT(IDT_NMI);
  MAKE_FAULT_IDT(IDT_BP);
  MAKE_FAULT_IDT(IDT_OF);
  MAKE_FAULT_IDT(IDT_BR);
  MAKE_FAULT_IDT(IDT_UD);
  MAKE_FAULT_IDT(IDT_NM);
  MAKE_FAULT_IDT(IDT_CSO);
  MAKE_FAULT_IDT(IDT_NP);
  MAKE_FAULT_IDT(IDT_SS);
  MAKE_FAULT_IDT(IDT_GP);
  MAKE_FAULT_IDT(IDT_PF);
  MAKE_FAULT_IDT(IDT_MF);
  MAKE_FAULT_IDT(IDT_AC);
  MAKE_FAULT_IDT(IDT_XF);
}

void printError(int es, int ds, int edi, int esi, int ebp, int _,
    int ebx, int edx, int ecx, int eax, int faultNumber, int errCode,
    int eip, int cs, int eflags, int esp, int ss, int cr2) {
  lprintf("Exception, IDT-number=%d. Core Dump:============================",
      faultNumber);
  lprintf("%%cs=%d\t%%ss=%d\t%%ds=%d\t%%es=%d", cs, ss, ds, es);
  lprintf("%%eax=0x%08X\t%%ebx=0x%08X\t%%ecx=0x%08X\t%%edx=0x%08X",
      eax, ebx, ecx, edx);
  lprintf("%%ebp=0x%08X\t%%esp=0x%08X\t%%esi=0x%08X\t%%edi=0x%08X",
      ebp, esp, esi, edi);
  lprintf("%%eip=0x%08X", eip);
  lprintf("EFLAGS=0x%08X\t%%cr2=0x%08X", eflags, cr2);
  lprintf("================================================================");
}

void unifiedErrorHandler(int es, int ds, int edi, int esi, int ebp, int _,
    int ebx, int edx, int ecx, int eax, int faultNumber, int errCode,
    int eip, int cs, int eflags, int esp, int ss) {
  int cr2 = get_cr2();
  printError(es, ds, edi, esi, ebp, _, ebx, edx, ecx, eax, faultNumber, errCode,
      eip, cs, eflags, esp, ss, cr2);
}
