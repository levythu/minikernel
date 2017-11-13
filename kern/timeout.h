/** @file timeout.h
 *
 *  @brief Timeout manager, now only controls sleep()
 *
 *  @author Leiyu Zhao
 */

#ifndef TIMEOUT_H
#define TIMEOUT_H

#include "bool.h"

// initialize timeout module, must be called in kenel INIT
void initTimeout();

// need to be called on each timer fires. It's reentrant safe, multithread safe,
// and the caller don't have to call it asynchronously (i.e. call it before ack
// timer interrupt)
void onTickEvent();

// Thread call it to sleep for at least given tics.
// It will not returned until the period has passed.
void sleepFor(uint32_t ticks);

// get the current ticks.
uint32_t getTicks();

#endif
