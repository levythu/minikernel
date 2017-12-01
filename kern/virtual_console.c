/** @file TODO
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
#include "keyboard_event.h"
#include "graphic_driver.h"
#include "console.h"
#include "scheduler.h"
#include "dbgconf.h"
#include "context_switch.h"
#include "kernel_stack_protection.h"
#include "hv.h"
#include "virtual_console_dev.h"
#include "virtual_console.h"

static virtualConsole* vcList[MAX_LIVE_VIRTUAL_CONSOLE];
static CrossCPULock latch;
static int currentVCN;

void* getVirtualConsole(int vcNumber) {
  assert(vcNumber != -1);
  GlobalLockR(&latch);
  virtualConsole* theVC = vcList[vcNumber];
  GlobalUnlockR(&latch);
  assert(theVC != NULL);
  assert(!theVC->dead);
  return theVC;
}

static void _useVirtualConsole(int vcNumber) {
  assert(vcNumber != -1);
  assert(vcList[vcNumber] != NULL);
  assert(!vcList[vcNumber]->dead);
  currentVCN = vcNumber;
  useVirtualKeyboard(vcNumber);
  useVirtualVideo(vcNumber);
}

void switchToVirtualConsole(int vcNumber) {
  GlobalLockR(&latch);
  // TODO Do something to the old
  _useVirtualConsole(vcNumber);
  lprintf("Virtual Console #%d activated, now %d processes inside.",
          vcNumber, vcList[vcNumber]->ref);
  GlobalUnlockR(&latch);
}

void switchNextVirtualConsole() {
  GlobalLockR(&latch);
  for (int i = 1; i < MAX_LIVE_VIRTUAL_CONSOLE; i++) {
    int thisVCN = (currentVCN + i) % MAX_LIVE_VIRTUAL_CONSOLE;
    if (vcList[thisVCN] == NULL) continue;
    switchToVirtualConsole(thisVCN);
    break;
  }
  GlobalUnlockR(&latch);
}

void referVirtualConsole(int vcNumber) {
  GlobalLockR(&latch);
  virtualConsole* theVC = vcList[vcNumber];
  GlobalUnlockR(&latch);
  assert(theVC != NULL);
  assert(!theVC->dead);
  __sync_fetch_and_add(&theVC->ref, 1);
}

void dereferVirtualConsole(int vcNumber) {
  GlobalLockR(&latch);
  virtualConsole* theVC = vcList[vcNumber];
  GlobalUnlockR(&latch);
  assert(theVC != NULL);
  assert(!theVC->dead);
  int currentRef = __sync_sub_and_fetch(&theVC->ref, 1);
  if (currentRef == 0) {
    theVC->dead = true;
  }
}

int newVirtualConsole() {
  virtualConsole* newVC = (virtualConsole*)smalloc(sizeof(virtualConsole));
  if (!newVC) {
    // No enough kernel memory
    return -1;
  }
  newVC->ref = 0;
  newVC->dead = false;
  initKeyboardEvent(newVC);
  initVirtualVideo(newVC);

  GlobalLockR(&latch);
  int i;
  for (i = 0; i < MAX_LIVE_VIRTUAL_CONSOLE; i++) {
    if (vcList[i] == NULL) break;
  }
  if (i == MAX_LIVE_VIRTUAL_CONSOLE) {
    // MAX_LIVE_VIRTUAL_CONSOLE reached
    GlobalUnlockR(&latch);
    sfree(newVC, sizeof(virtualConsole));
    return -1;
  }
  vcList[i] = newVC;
  newVC->vcNumber = i;
  GlobalUnlockR(&latch);
  return i;
}

// let kernel call it on startup!
// It must happen before register keyboard driver
int initVirtualConsole() {
  install_graphic_driver();

  for (int i = 0; i < MAX_LIVE_VIRTUAL_CONSOLE; i++) {
    vcList[i] = NULL;
  }
  initCrossCPULock(&latch);
  // 1. setting up the default vc
  int initConsole = newVirtualConsole();
  _useVirtualConsole(initConsole);
  return initConsole;
}
