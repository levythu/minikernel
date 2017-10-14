/** @file timer_driver.h
 *
 *  @brief The definition for functions and macros for timer driver.
 *  Note that all timer interfaces are declared in p1kern.h, so here we do
 *  not do it again.
 *
 *  @author Leiyu Zhao (leiyuz)
 */

#ifndef TIMER_DRIVER_H
#define TIMER_DRIVER_H

#include <stdio.h>

#include "bool.h"

// The type for timer callback
typedef void (*TimerCallback)(unsigned int);

// Install the timer driver. For any errors return negative integer;
// otherwise return 0; The parameter passing in is the callback for each
// triggers (every 10ms). The callback is synchronous, which means that it
// mustn't take long.
extern int install_timer_driver(TimerCallback callback);

#endif
