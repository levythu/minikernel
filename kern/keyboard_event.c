/** @file kayboard.c
 *
 *  @brief TODO
 *
 *  TODO
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

static kmutex keyboardHolder;

static bool waitingForAnyChar; // true for char, false for string
static tcb* eventWaiter;
static CrossCPULock latch;

// let kernel call it on startup!
// It must happen before register keyboard driver
void initKeyboardEvent() {
  kmutexInit(&keyboardHolder);
  initCrossCPULock(&latch);
  eventWaiter = NULL;
}

void occupyKeyboard() {
  kmutexWLock(&keyboardHolder);
}

void releaseKeyboard() {
  kmutexWUnlock(&keyboardHolder);
}

// Must be called when occupying keyboard
int getcharBlocking() {
  while (true) {
    GlobalLockR(&latch);
    int ch = fetchCharEvent();
    if (ch >= 0) {
      GlobalUnlockR(&latch);
      return ch;
    }
    assert(eventWaiter == NULL);

    // okay, it's time to sleep
    tcb* currentThread = findTCB(getLocalCPU()->runningTID);
    eventWaiter = currentThread;
    waitingForAnyChar = true;
    currentThread->descheduling = true;
    currentThread->status = THREAD_BLOCKED;
    removeFromXLX(currentThread);
    GlobalUnlockR(&latch);

    // We sleep with keyboardHolder, since we are still the one that listens to
    // keyboard
    yieldToNext();
  }
  // No we are not gonna reach here
  return -1;
}

// Must be called when occupying keyboard
// maxlen cannot be zero
// TODO: remove non-printable char
int getStringBlocking(char* space, int maxlen) {
  int currentLen = 0;
  bool preexist = true;
  while (true) {
    GlobalLockR(&latch);
    while (true) {
      int ch = fetchCharEvent();
      if (ch >= 0 && preexist) {
        putbyte(ch);
      }
      if (ch == '\b') {
        if (currentLen > 0) currentLen--;
      } else if (ch == '\n') {
        space[currentLen++] = ch;
        if (currentLen < maxlen) {
          space[currentLen++] = 0;
        }
        GlobalUnlockR(&latch);
        return currentLen;
      } else if (ch >= 0) {
        space[currentLen++] = ch;
        if (currentLen == maxlen) {
          GlobalUnlockR(&latch);
          return maxlen;
        }
      } else {
        break;
      }
    }
    preexist = false;
    assert(eventWaiter == NULL);

    // okay, it's time to sleep
    tcb* currentThread = findTCB(getLocalCPU()->runningTID);
    eventWaiter = currentThread;
    waitingForAnyChar = false;
    currentThread->descheduling = true;
    currentThread->status = THREAD_BLOCKED;
    removeFromXLX(currentThread);
    GlobalUnlockR(&latch);

    // We sleep with keyboardHolder, since we are still the one that listens to
    // keyboard
    yieldToNext();
  }
  // No we are not gonna reach here
  return -1;
}

// TODO: remove non-printable char
void onKeyboardSync(int ch) {
  GlobalLockR(&latch);
  if (eventWaiter && !waitingForAnyChar) {
    putbyte(ch);
  }
  GlobalUnlockR(&latch);
}

void onKeyboardAsync(int ch) {
  KERNEL_STACK_CHECK;
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  #ifdef CONTEXT_SWTICH_ON_RIGHT_KEY
    if (ch == KHE_ARROW_RIGHT) {
      if (currentThread && !currentThread->descheduling) {
        yieldToNext();
      }
      return;
    }
  #endif
  GlobalLockR(&latch);
  if (eventWaiter) {
    if (waitingForAnyChar || ch == '\n') {
      tcb* local = eventWaiter;
      eventWaiter = NULL;
      GlobalUnlockR(&latch);

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
  GlobalUnlockR(&latch);
}
