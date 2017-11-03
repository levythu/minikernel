/** @file fault.h
 *
 *  @brief TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef FAULT_H
#define FAULT_H

void printError(int es, int ds, int edi, int esi, int ebp, int _,
    int ebx, int edx, int ecx, int eax, int faultNumber, int errCode,
    int eip, int cs, int eflags, int esp, int ss, int cr2);

void registerFaultHandler();

#endif
