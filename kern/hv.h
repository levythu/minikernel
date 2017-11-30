/** @file TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef HV_H
#define HV_H

#include "hvseg.h"
#include "hvlife.h"
#include "hv_hpcall.h"

void initHypervisor();

void hv_CallMeOnTick(HyperInfo* info);

bool HyperFaultHandler(int es, int ds, int edi, int esi, int ebp,
    int ebx, int edx, int ecx, int eax, int faultNumber, int errCode,
    int eip, int cs, int eflags, int esp, int ss, int cr2);

#endif
