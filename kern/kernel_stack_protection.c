/** @file kernel_stack_protection.c
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
#include "common_kern.h"
#include "pm.h"
#include "vm.h"
#include "bool.h"
#include "asm_wrapper.h"
#include "process.h"
#include "cpu.h"

void checkKernelStackOverflow() {
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  if (!currentThread) return;
  uint32_t retAddr = (uint32_t)get_retAddr();
  if (retAddr < currentThread->kernelStackPage ||
      retAddr >= currentThread->kernelStackPage + PAGE_SIZE) {
    panic("Current retAddr = 0x%08lx, not falling in kernel stack "
          "[0x%08lx, 0x%08lx). Suspecting kernel stack curruption.",
          retAddr, currentThread->kernelStackPage,
          currentThread->kernelStackPage + PAGE_SIZE);
  }
}
