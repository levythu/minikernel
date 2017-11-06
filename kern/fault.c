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
#include <string.h>
#include <x86/asm.h>
#include <x86/cr.h>
#include <x86/idt.h>

#include "common_kern.h"
#include "pm.h"
#include "vm.h"
#include "bool.h"
#include "cpu.h"
#include "process.h"
#include "fault_handler_internal.h"
#include "fault_handler_user.h"
#include "asm_wrapper.h"
#include "int_handler.h"
#include "zeus.h"
#include "console.h"

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

// TODO handle mesterious fault!
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

#define FAULT_ACTION(_name) \
  bool _name(int es, int ds, int edi, int esi, int ebp, \
      int ebx, int edx, int ecx, int eax, int faultNumber, int errCode, \
      int eip, int cs, int eflags, int esp, int ss, int cr2)

#define ON(_cond, _name) \
  if (_cond) {  \
    if (_name(es, ds, edi, esi, ebp, ebx, edx, ecx, eax, faultNumber,\
      errCode, eip, cs, eflags, trueESP, trueSS, cr2)) return;  \
  }

/*****************************************************************************/
// Now it's all about fault handlers

// Print the fault out
FAULT_ACTION(printError) {
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
  return false;
}

// This handler is used for upgrade an readonly ZFOD block to RW All zero block
FAULT_ACTION(ZFODUpgrader) {
  if ((uint32_t)cr2 < USER_MEM_START) {
    // Easy, access kernel memory, not ZFOD'd
    return false;
  }

  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  if (!currentThread) {
    return false; // non-running thread then pass on
  }

  // Use kmutexWLockForce to get the lock recursively and adaptively.
  // Since when write happens RLock cannot be acquired, it's safe to call so
  kmutexStatus oldMemLockStatus;
  kmutexWLockForce(&currentThread->process->memlock,
      &currentThread->memLockStatus, &oldMemLockStatus);

  PTE* pteEntry = searchPTEntryPageDirectory(currentThread->process->pd,
      PE_DECODE_ADDR(cr2));
  if (!pteEntry) {
    // This is a out-of-address access, not ZFOD
    kmutexWUnlockForce(&currentThread->process->memlock,
        &currentThread->memLockStatus, oldMemLockStatus);
    return false;
  }
  if (!isZFOD(PE_DECODE_ADDR(*pteEntry))) {
    if (PE_IS_WRITABLE(*pteEntry)) {
      // looks like a good address, maybe it's due to other threads have fix the
      // ZFOD. Give another chance, try again
      kmutexWUnlockForce(&currentThread->process->memlock,
          &currentThread->memLockStatus, oldMemLockStatus);
      return true;
    }
    // accessing a readonly addr, leave it to others
    kmutexWUnlockForce(&currentThread->process->memlock,
        &currentThread->memLockStatus, oldMemLockStatus);
    return false;
  }
  // Handle ZFOD!
  uint32_t newPage = upgradeUserMemPageZFOD(PE_DECODE_ADDR(*pteEntry));
  *pteEntry = PTE_CLEAR_ADDR(*pteEntry) | PE_WRITABLE(1) | newPage;
  invalidateTLB(cr2);
  memset((void*)PE_DECODE_ADDR(cr2), 0, PAGE_SIZE);

  kmutexWUnlockForce(&currentThread->process->memlock,
      &currentThread->memLockStatus, oldMemLockStatus);

  // Good! Retry and everything should be fine!
  return true;
}

FAULT_ACTION(UserModeErrorSWE) {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  assert(currentThread);

  ureg_t uregs;
  uregs.cause = translateIDTNumToCause(faultNumber);
  uregs.cr2 = cr2;
  uregs.ds = ds;
  uregs.es = es;
  uregs.edi = edi;
  uregs.esi = esi;
  uregs.ebp = ebp;
  uregs.ebx = ebx;
  uregs.edx = edx;
  uregs.ecx = ecx;
  uregs.eax = eax;
  uregs.error_code = errCode;
  uregs.eip = eip;
  uregs.cs = cs;
  uregs.eflags = eflags;
  uregs.esp = esp;
  uregs.ss = ss;
  // It should never return on succ
  return makeRegisterHandlerStackAndGo(currentThread, &uregs);
}

FAULT_ACTION(UserModeErrorCrash) {
  // Crash! It's equal to vanish
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  lprintf("Thread #%d of process #%d aborts for exception.\n",
      getLocalCPU()->runningTID, getLocalCPU()->runningPID);
  // one way trip
  terminateThread(currentThread);
  panic("What.... I should've terminated");
  return true;
}

FAULT_ACTION(CatchAllHandler) {
  panic("Uncaught exception. See descriptions above.");
  return true;
}


void unifiedErrorHandler(int es, int ds, int edi, int esi, int ebp,
    int espOnCurrentStack,
    int ebx, int edx, int ecx, int eax, int faultNumber, int errCode,
    int eip, int cs, int eflags, int esp, int ss) {
  int trueESP = eip >= USER_MEM_START ? esp : espOnCurrentStack;
  int trueSS = eip >= USER_MEM_START ? ss : get_ss();
  int cr2 = get_cr2();

  // Area for OS tricks that's hidden from anyone else
  ON(faultNumber == IDT_PF, ZFODUpgrader);

  // General info for debugger
  ON(true, printError);

  // Huh, user code! What are you doing!
  ON(eip >= USER_MEM_START, UserModeErrorSWE);
  ON(eip >= USER_MEM_START, UserModeErrorCrash);

  // Hmmmmmm it's me that screwed up. Let's panic and let Levy debug it
  ON(true, CatchAllHandler);
}
