/** @file zeus.h
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#ifndef ZEUS_H
#define ZEUS_H

pcb* SpawnProcess(tcb** firstThread);

int LoadELFToProcess(pcb* proc, tcb* firstThread, const char* fileName,
    ArgPackage* argpkg, uint32_t* eip, uint32_t* esp);

int forkProcess(tcb* currentThread);

int execProcess(tcb* currentThread, const char* filename, ArgPackage* argpkg);

void terminateThread(tcb* currentThread);

int waitThread(tcb* currentThread, int* returnCodeAddr);

#endif
