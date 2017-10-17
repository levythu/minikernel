/** @file cpu.h
 *
 *  @brief TODO

 *
 *  @author Leiyu Zhao
 */

#ifndef CONTEXT_SWITCH_H
#define CONTEXT_SWITCH_H

#include "ureg.h"

void switchTheWorld(ureg_t* oldURegSavePlace, ureg_t *newUReg);

#endif
