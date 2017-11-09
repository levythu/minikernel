/** @file timeout.h
 *
 *  @brief TODO
 *
 *  @author Leiyu Zhao
 */

#ifndef TIMEOUT_H
#define TIMEOUT_H

#include "bool.h"

void initTimeout();
void onTickEvent();
void sleepFor(uint32_t ticks);
uint32_t getTicks();

#endif
