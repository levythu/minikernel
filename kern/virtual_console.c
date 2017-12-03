/** @file virtual_console.c
 *
 *  @brief Implementation of virtual console helper functions
 *
 *  @author Leiyu Zhao
 */

#include <stdio.h>
#include <simics.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <x86/keyhelp.h>
#include <x86/video_defines.h>

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

// The list of virtual console, NULL for a invalid console
static virtualConsole* vcList[MAX_LIVE_VIRTUAL_CONSOLE];
static CrossCPULock latch;
static int currentVCN;

void reportVC() {
  GlobalLockR(&latch);
    lprintf("├ Virtual Console List");
  int totCount = 0;
  for (int i = 0; i < MAX_LIVE_VIRTUAL_CONSOLE; i++) {
    if (vcList[i] == NULL) continue;
    totCount++;
    lprintf("│ ├ [Console #%d] Shared by %d procs", i, vcList[i]->ref);
  }
    lprintf("│ └ Total %d screens", totCount);
  GlobalUnlockR(&latch);
}

void* getVirtualConsole(int vcNumber) {
  assert(vcNumber != -1);
  GlobalLockR(&latch);
  virtualConsole* theVC = vcList[vcNumber];
  GlobalUnlockR(&latch);
  assert(theVC != NULL);
  return theVC;
}

// Use a virtual console, must be protected under latch
// Or in init phase
static void _useVirtualConsole(int vcNumber) {
  assert(vcNumber != -1);
  assert(vcList[vcNumber] != NULL);
  currentVCN = vcNumber;
  useVirtualKeyboard(vcNumber);
  useVirtualVideo(vcNumber);
}

void switchToVirtualConsole(int vcNumber) {
  virtualConsole* toFree = NULL;
  GlobalLockR(&latch);
  assert(currentVCN != vcNumber);
  if (vcList[currentVCN]->dead) {
    toFree = vcList[currentVCN];
    vcList[currentVCN] = NULL;
  }
  _useVirtualConsole(vcNumber);
  lprintf("Virtual Console #%d activated, now %d processes inside.",
          vcNumber, vcList[vcNumber]->ref);
  GlobalUnlockR(&latch);
  if (toFree) {
    sfree(toFree, sizeof(virtualConsole));
  }
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
    // It's dead now. We can draw quitting info and finnaly set dead flag.
    // DO NOT set dead before finish drawing -- someone else may clear the VC

    const static char* quitInfo1 =
        "\nThere's no process running inside. Press [TAB] to switch out.\n";
    const static char* quitInfo2 =
        "WARNING: once you switch, you can **NEVER** get back to this console!";

    set_term_color(vcNumber, BGND_BLACK | FGND_CYAN);
    putbytes(vcNumber, quitInfo1, strlen(quitInfo1));
    set_term_color(vcNumber, BGND_BLACK | FGND_RED);
    putbytes(vcNumber, quitInfo2, strlen(quitInfo2));

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
