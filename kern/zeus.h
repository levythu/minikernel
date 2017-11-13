/** @file zeus.h
 *
 *  @brief Zeus control every life (process and thread)
 *
 *  This module is used for higher level process and thread lifecycle control.
 *  It contains the spawn, fork, exec, death (a great part for death is
 *  deligated to reaper, see reaper.c) for process and thread. Also it
 *  coordinates different modules to achieve those functionalities
 *
 *  @author Leiyu Zhao
 */

#ifndef ZEUS_H
#define ZEUS_H

// Create a process, together with the first thread. And init basic members for
// pcb/tcb. When returned, the current CPU will own first thread
pcb* SpawnProcess(tcb** firstThread);

// Load an ELF to given process and thread (don't have to be running thread)
// Given the arg package.
// Will return the start eip and esp for mode switch.
// On failure returns -1
int LoadELFToProcess(pcb* proc, tcb* firstThread, const char* fileName,
    ArgPackage* argpkg, uint32_t* eip, uint32_t* esp);

// The internal implementation of fork. Will result in two threads (returns)
// Parent process will get the first thread TID of child process
// Child process will get 0
// On failure returns -1
int forkProcess(tcb* currentThread);

int execProcess(tcb* currentThread, const char* filename, ArgPackage* argpkg);

// Terminate thread. This function never returns
void terminateThread(tcb* currentThread);

int waitThread(tcb* currentThread, int* returnCodeAddr);

int forkThread(tcb* currentThread);

// Create a thread under given process, and init basic members for tcb.
// Note it will not increment proc->numThread
tcb* SpawnThread(pcb* proc);

#endif
