/** @file mutex_type.h
 *  @brief This file defines the type for mutexes.
 */

#ifndef _MUTEX_TYPE_H
#define _MUTEX_TYPE_H

#include <stdbool.h>

// For outside caller you shouldn't care about these knobs. For detail please
// refer to mutex.c
#define MUTEX_MAX_NON_SPINWAIT_THREADS 64

typedef struct mutex {
    int ticketToTID[MUTEX_MAX_NON_SPINWAIT_THREADS];
    int ticketCalled[MUTEX_MAX_NON_SPINWAIT_THREADS];
    int nextTicketAvailable;
    int ticketOnCall;
} mutex_t;

#endif /* _MUTEX_TYPE_H */
