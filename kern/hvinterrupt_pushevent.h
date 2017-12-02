/** @file hvinterrupt_pushevent.h
 *
 *  @brief Define some constants used by pushevents
 *
 *  @author Leiyu Zhao
 */

#ifndef HV_INTERRUPT_TIMER_H
#define HV_INTERRUPT_TIMER_H

#include "hvinterrupt.h"

// Define how many context switch will generate one virtual timer tick for
// a guest.
// It should be larger than one because in each context switch, only one delayed
// interrupt is pushed (see README), when it's one, the interrupt queue can
// never be drained
#define GENERATE_TIC_EVERY_N_TIMESLICE 2

#endif
