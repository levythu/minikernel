/** @file reaper.h
 *
 *  @brief Death walks among you
 *
 *  Reaper handles the death of a thread and process. It collaborate with zeus
 *  to be the core function module in OS
 *
 *  @author Leiyu Zhao
 */

#ifndef REAPER_H
#define REAPER_H

#include "vm.h"

void freeUserspace(PageDirectory pd);

void reapThread(tcb* targetThread);

// Called by Zeus. When the thread is dying. After this, the caller should set
// current thread to THREAD_DEAD and deschedule
void suicideThread(tcb* targetThread);

#endif
