/** @file fault.h
 *
 *  @brief Declare exposed functions for fault handling module
 *
 *  @author Leiyu Zhao
 */

#ifndef FAULT_H
#define FAULT_H

#define FAULT_ACTION(_name) \
  bool _name(int es, int ds, int edi, int esi, int ebp, \
      int ebx, int edx, int ecx, int eax, int faultNumber, int errCode, \
      int eip, int cs, int eflags, int esp, int ss, int cr2)

// Just for debug
bool printError(int es, int ds, int edi, int esi, int ebp,
    int ebx, int edx, int ecx, int eax, int faultNumber, int errCode,
    int eip, int cs, int eflags, int esp, int ss, int cr2);

// Make fault handler ready. Need to be called in kernel init phase (before go
// into ring3 for the 1st time).
void registerFaultHandler();

#endif
