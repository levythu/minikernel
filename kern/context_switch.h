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

// Similar to setjump, will make a snapshot of common register and store it to
// savePlace, so that it can be passed to switchTheWorld to restore.
// It also snapshot some memory [memToSnapshot, memToSnapshot + size) to
// [memDst, memToSnapshot + size), so that the most realtime memory can be
// preserved (it is used to duplicate kernel stack on fork)
// Return: false if it is making the snapshot; true if it is jumped back from
// a snapshot
bool checkpointTheWorld(ureg_t* savePlace,
    uint32_t memToSnapshot, uint32_t memDst, uint32_t size);

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
