/** @file context_switch.h
 *
 *  @brief All infrastructure definitions for context switch
 *
 *
 *  @author Leiyu Zhao
 */

#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

#include "ureg.h"
#include "process.h"
#include "bool.h"

// The kernel of the kernel, switch the whole world to some other context
// Only regular registers (plus eip) are switched, except %eax %ecx %edx
// Old register is stored in oldURegSavePlace. And the eip is stored so that
// when it is resumed, it will return from the function with return value = hint
// New register is adopted.
int switchTheWorld(ureg_t* oldURegSavePlace, ureg_t *newUReg, int hint);

bool checkpointTheWorld(ureg_t* savePlace);

// Swtich to the thread pointed by parameter, also switch the process if needed
// NOTE Current CPU must own the current thread and the thread to switch
// It will do several things:
// - Switch the process, if possible
// - Switch the kernel stack
// - Maintain the states for two TCBs
// - Change the local CPU settings
// - Switch the world to the new thread
// It's also the entry when a descheduled thread is switched-on, and it will
// looks like returnning from switchTheWorld() call.
// - Disown the former thread
//
// When it returns, you have no idea about whether thread still exist, you
// MUST NOT access it anymore. Unless you know it.
//
// The function is interrupt-safe (it acquires local lock)
void swtichToThread(tcb* thread);

#endif
