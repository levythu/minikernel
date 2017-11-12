/** @file fault_handler_user.c
 *
 *  @brief User-caused fault handler and related helper functions
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>

#include "x86/asm.h"
#include "x86/idt.h"
#include "x86/cr.h"
#include "x86/eflags.h"
#include "common_kern.h"
#include "pm.h"
#include "vm.h"
#include "bool.h"
#include "process.h"
#include "cpu.h"
#include "source_untrusted.h"
#include "mode_switch.h"

// Register a user-space fault handler to one thread. Thread should be owned.
// Won't do validation on any param -- the caller should do it
void registerUserFaultHandler(tcb* thread, uint32_t esp3, uint32_t eip,
    uint32_t customArg) {
  thread->faultHandler = eip;
  thread->customArg = customArg;
  thread->faultStack = esp3;
}

// Remove a user-space fault handler from one thread. Thread should be owned.
void deregisterUserFaultHandler(tcb* thread) {
  thread->faultHandler = 0;
  thread->customArg = 0;
  thread->faultStack = 0;
}

// Helper function, translate a idtnumber (provided in kernel fault handler)
// to a ureg_t.cause
int translateIDTNumToCause(int idtNumber) {
  #define MAKE_MAP(_idtDst, _causeDst) \
    if (idtNumber == (_idtDst)) return (_causeDst)
  MAKE_MAP(IDT_DE, SWEXN_CAUSE_DIVIDE);
  MAKE_MAP(IDT_DB, SWEXN_CAUSE_DEBUG);
  MAKE_MAP(IDT_BP, SWEXN_CAUSE_BREAKPOINT);
  MAKE_MAP(IDT_OF, SWEXN_CAUSE_OVERFLOW);
  MAKE_MAP(IDT_BR, SWEXN_CAUSE_BOUNDCHECK);
  MAKE_MAP(IDT_UD, SWEXN_CAUSE_OPCODE);
  MAKE_MAP(IDT_NM, SWEXN_CAUSE_NOFPU);
  MAKE_MAP(IDT_NP, SWEXN_CAUSE_SEGFAULT);
  MAKE_MAP(IDT_SS, SWEXN_CAUSE_STACKFAULT);
  MAKE_MAP(IDT_GP, SWEXN_CAUSE_PROTFAULT);
  MAKE_MAP(IDT_PF, SWEXN_CAUSE_PAGEFAULT);
  MAKE_MAP(IDT_MF, SWEXN_CAUSE_FPUFAULT);
  MAKE_MAP(IDT_AC, SWEXN_CAUSE_ALIGNFAULT);
  MAKE_MAP(IDT_XF, SWEXN_CAUSE_SIMDFAULT);
  return -1;
}

// Consume uregs, return to ring3 with target registers.
// Should never return on success, or false on failure
bool makeRegisterHandlerStackAndGo(tcb* thread, ureg_t* uregs) {
  if (thread->faultHandler == 0) {
    return false;
  }
  kmutexWLockRecord(&thread->process->memlock, &thread->memLockStatus);
  // Give place for some parameters
  int SWEXN_minimalStackSize = sizeof(ureg_t) + sizeof(uint32_t) * 5;
  if (!verifyUserSpaceAddr(thread->faultStack - 2 - SWEXN_minimalStackSize,
                           thread->faultStack - 2, true)) {
     kmutexWUnlockRecord(&thread->process->memlock, &thread->memLockStatus);
      return false;
  }
  uint32_t esp = thread->faultStack - 1;

  esp -= sizeof(ureg_t);
  *(ureg_t*)esp = *uregs;
  uint32_t uregLoc = esp;

  esp -= 4;
  *(uint32_t*)esp = uregLoc;

  esp -= 4;
  *(uint32_t*)esp = thread->customArg;

  esp -= 4;
  *(uint32_t*)esp = 0xdeadbeef; // return addr

  kmutexWUnlockRecord(&thread->process->memlock, &thread->memLockStatus);

  if (!(get_eflags() & EFL_IF)) {
    panic("Oooops! Lock skews... current lock layer = %d",
        getLocalCPU()->currentMutexLayer);
  }
  uint32_t neweflags =
      (get_eflags() | EFL_RESV1 | EFL_IF) & ~EFL_AC;
  uint32_t eip = thread->faultHandler;
  deregisterUserFaultHandler(thread);
  switchToRing3X(esp, neweflags, eip, 0, 0, 0, 0, 0, 0, 0);
  // Should never reach
  return false;
}
