/** @file reaper.h
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#ifndef REAPER_H
#define REAPER_H

#include "vm.h"

void freeUserspace(PageDirectory pd);

void reapThread(tcb* targetThread);

#endif
