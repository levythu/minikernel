/** @file kayboard.c
 *
 *  @brief keyboard event handler.
 *
 *  This module is decoupled from keyboard handler. And kernel should attach it
 *  to keyboard event to make things work,
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <x86/keyhelp.h>

#include "vm.h"
#include "bool.h"
#include "cpu.h"
#include "kmutex.h"
#include "process.h"
#include "keyboard_driver.h"
#include "graphic_driver.h"
#include "console.h"
#include "scheduler.h"
#include "dbgconf.h"
#include "context_switch.h"
#include "kernel_stack_protection.h"
#include "hv.h"
#include "virtual_console_dev.h"
#include "virtual_console.h"

static virtualConsole* currentVC = NULL;

void useVirtualKeyboard(int vcn) {
  currentVC = (virtualConsole*)getVirtualConsole(vcn);
}

void initKeyboardEvent(void* _vc) {
  virtualConsole* vc = (virtualConsole*)_vc;
  kmutexInit(&vc->i.keyboardHolder);
  initCrossCPULock(&vc->i.latch);
  vc->i.eventWaiter = NULL;
  initMultiplexer(&vc->i.kbMul);
}

intMultiplexer* getKeyboardMultiplexer(int vcn) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  return &vc->i.kbMul;
}

void occupyKeyboard(int vcn) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  kmutexWLock(&vc->i.keyboardHolder);
}

void releaseKeyboard(int vcn) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  kmutexWUnlock(&vc->i.keyboardHolder);
}

// Must be called when occupying keyboard
int getcharBlocking(int vcn) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  while (true) {
    GlobalLockR(&vc->i.latch);
    int ch = fetchCharEvent();
    if (ch >= 0) {
      GlobalUnlockR(&vc->i.latch);
      return ch;
    }
    assert(vc->i.eventWaiter == NULL);

    // okay, it's time to sleep
    tcb* currentThread = findTCB(getLocalCPU()->runningTID);
    vc->i.eventWaiter = currentThread;
    vc->i.waitingForAnyChar = true;
    currentThread->descheduling = true;
    currentThread->status = THREAD_BLOCKED;
    removeFromXLX(currentThread);
    GlobalUnlockR(&vc->i.latch);

    // We sleep with keyboardHolder, since we are still the one that listens to
    // keyboard
    yieldToNext();
  }
  // No we are not gonna reach here
  return -1;
}

// Must be called when occupying keyboard
// maxlen cannot be zero
int getStringBlocking(int vcn, char* space, int maxlen) {
  virtualConsole* vc = (virtualConsole*)getVirtualConsole(vcn);
  int currentLen = 0;
  bool preexist = true;
  while (true) {
    GlobalLockR(&vc->i.latch);
    while (true) {
      int ch = fetchCharEvent();
      if (ch >= 0 && preexist) {
        putbyte_(vcn, ch);
      }
      if (ch == '\b') {
        if (currentLen > 0) currentLen--;
      } else if (ch == '\n') {
        space[currentLen++] = ch;
        if (currentLen < maxlen) {
          space[currentLen++] = 0;
        }
        GlobalUnlockR(&vc->i.latch);
        return currentLen;
      } else if (ch >= 0) {
        space[currentLen++] = ch;
        if (currentLen == maxlen) {
          GlobalUnlockR(&vc->i.latch);
          return maxlen;
        }
      } else {
        break;
      }
    }
    preexist = false;
    assert(vc->i.eventWaiter == NULL);

    // okay, it's time to sleep
    tcb* currentThread = findTCB(getLocalCPU()->runningTID);
    vc->i.eventWaiter = currentThread;
    vc->i.waitingForAnyChar = false;
    currentThread->descheduling = true;
    currentThread->status = THREAD_BLOCKED;
    removeFromXLX(currentThread);
    GlobalUnlockR(&vc->i.latch);

    // We sleep with keyboardHolder, since we are still the one that listens to
    // keyboard
    yieldToNext();
  }
  // No we are not gonna reach here
  return -1;
}

// In readline mode, print the char in synchronously to avoid re-order
void onKeyboardSync(int ch) {
  virtualConsole* vc = currentVC;
  GlobalLockR(&vc->i.latch);
  if (vc->i.eventWaiter && !vc->i.waitingForAnyChar) {
    putbyte_(vc->vcNumber, ch);
  }
  GlobalUnlockR(&vc->i.latch);
}

// For the rest of work (awakening the waiter), out-of-order is acceptable
void onKeyboardAsync(int ch) {
  KERNEL_STACK_CHECK;
  virtualConsole* vc = currentVC;

  hvInt ev;
  ev.intNum = KEY_IDT_ENTRY;
  ev.spCode = ch;
  ev.cr2 = 0;
  broadcastIntTo(&vc->i.kbMul, ev);


  if (ch == KHE_TAB) {
    switchNextVirtualConsole();
    return;
  }

  if (ch < 0) return;
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  #ifdef CONTEXT_SWTICH_ON_RIGHT_KEY
    if (ch == KHE_ARROW_RIGHT) {
      if (currentThread && !currentThread->descheduling) {
        yieldToNext();
      }
      return;
    }
  #endif
  GlobalLockR(&vc->i.latch);
  if (vc->i.eventWaiter) {
    if (vc->i.waitingForAnyChar || ch == '\n') {
      tcb* local = vc->i.eventWaiter;
      vc->i.eventWaiter = NULL;
      GlobalUnlockR(&vc->i.latch);

      LocalLockR();
      assert(local->status == THREAD_BLOCKED);
      // Own the target thread for context switch. Since it's blocked, no one
      // should really own it, other cores may be accessing it temprorarily.
      // Should never loop on single core
      while (!__sync_bool_compare_and_swap(
          &local->owned, THREAD_NOT_OWNED, THREAD_OWNED_BY_THREAD))
        ;
      local->status = THREAD_RUNNABLE;
      addToXLX(local);
      if (currentThread && !currentThread->descheduling) {
        swtichToThread_Prelocked(local);
      } else {
        local->owned = THREAD_NOT_OWNED;
        LocalUnlockR();
      }

      return;
    }
  }
  GlobalUnlockR(&vc->i.latch);
}
