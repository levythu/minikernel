/** @file cpu.h
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

#include "ureg.h"
#include "process.h"

int switchTheWorld(ureg_t* oldURegSavePlace, ureg_t *newUReg, int hint);

// Must own the thread, and, of course, the current thread.
// It will disown current thread
void swtichToThread(tcb* thread);

#endif
