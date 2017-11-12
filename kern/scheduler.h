/** @file scheduler.h
 *
 *  @brief Round robin scheduler
 *
 *  Exposes yieldToNext, which is called in almost all context switch case
 *  (except targeted awakening) to switch to the next runnable thread.
 *
 *  It fetches runnable threads from XLX (see process.h), which skips BLOCKED
 *  threads
 *
 *  It also triggers reaper when a dead thread is found.
 *
 *  Note that the work done in scheduler is non-trivial, and during the run
 *  another timer event may trigger a new scheduler on top of it -- It's safe,
 *  however, if such things happens time after time to finally overflow the
 *  kernel stack, stack protector will beep. And we need to adjust down the
 *  timer fire rate.
 *
 *  @author Leiyu Zhao
 */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "process.h"

void yieldToNext();
tcb* pickNextRunnableThread(tcb* currentThread);

#endif
