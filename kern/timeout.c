/** @file timeout.c
 *
 *  @brief TODO
 *
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
#include "cpu.h"
#include "process.h"
#include "scheduler.h"

typedef struct _timeoutStatus {
  tcb* thread;
  uint32_t waitUntil;
  struct _timeoutStatus* next;
} timeoutStatus;

static timeoutStatus* sleepers;
static CrossCPULock latch;
static uint32_t tickVal;

static void insertTimeoutStatus(timeoutStatus* toInsert);
static timeoutStatus* tryRemoveTimeoutStatus();
static void alarm();

// Must happens before any tickevent happens
void initTimeout() {
  initCrossCPULock(&latch);
  tickVal = 0;
}

void onTickEvent() {
  __sync_fetch_and_add(&tickVal, 1);
  alarm();
}

uint32_t getTicks() {
  return tickVal;
}

void sleepFor(uint32_t ticks) {
  if (ticks == 0) return;
  tcb* currentThread = findTCB(getLocalCPU()->runningTID);
  assert(currentThread != NULL);

  timeoutStatus ts;
  ts.thread = currentThread;
  ts.waitUntil = tickVal + ticks;
  GlobalLockR(&latch);
  insertTimeoutStatus(&ts);
  currentThread->descheduling = true;
  currentThread->status = THREAD_BLOCKED;
  removeFromXLX(currentThread);
  GlobalUnlockR(&latch);
  // It's time to sleep
  yieldToNext();
}

/************************************************/

// This function itself does not disable interrupt, instead, it runs
// tryRemoveTimeoutStatus time after time to wake all up until no one needs to
// be waked. This function use something to keep running at single-instance
// We don't switch to target thread imediately for two reasons: 1. there's slack
// for a thread to be run; 2. we may hava somebody else to awake
static int alarmSingleInstanceGuard = 0;
static void alarm() {
  if (!__sync_bool_compare_and_swap(&alarmSingleInstanceGuard, 0, 1)) {
    // someone is still running, quit
    return;
  }
  while (true) {
    timeoutStatus* ts = tryRemoveTimeoutStatus();
    if (!ts) break;
    assert(ts->thread->status == THREAD_BLOCKED);
    addToXLX(ts->thread);
    ts->thread->status = THREAD_RUNNABLE;
  }
  alarmSingleInstanceGuard = 0;
}

static void insertTimeoutStatus(timeoutStatus* toInsert) {
  GlobalLockR(&latch);
  timeoutStatus** fromPtr = &sleepers;
  while (true) {
    if (*fromPtr == NULL) {
      // Now it's the largest waiter
      break;
    }
    if (toInsert->waitUntil < (*fromPtr)->waitUntil) {
      // Okay looks like a good spot, I'm right behind you
      break;
    }
    fromPtr = &(*fromPtr)->next;
  }
  toInsert->next = *fromPtr;
  *fromPtr = toInsert;
  GlobalUnlockR(&latch);
}

static timeoutStatus* tryRemoveTimeoutStatus() {
  GlobalLockR(&latch);
  if (sleepers == NULL) {
    GlobalUnlockR(&latch);
    return NULL;
  }
  if (sleepers->waitUntil > tickVal) {
    // the smallest sleeper shouln't be waked.
    GlobalUnlockR(&latch);
    return NULL;
  }
  timeoutStatus* ret = sleepers;
  sleepers = sleepers->next;
  GlobalUnlockR(&latch);
  return ret;
}
