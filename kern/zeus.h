/** @file zeus.h
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#ifndef ZEUS_H
#define ZEUS_H

pcb* SpawnProcess(tcb** firstThread);
int LoadELFToProcess(pcb* proc, tcb* firstThread, const char* fileName);

#endif