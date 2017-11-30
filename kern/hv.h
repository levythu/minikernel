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

#endif
