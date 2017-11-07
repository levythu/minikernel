/** @file cond_type.h
 *  @brief This file defines the type for condition variables.
 */

#ifndef _COND_TYPE_H
#define _COND_TYPE_H

#define CONDVAR_MAX_NON_SPINWAIT_THREADS 256

// For outside caller you shouldn't care about these knobs. For detail please
// refer to condvar.c
typedef struct cond {
    mutex_t mutex;
    int ticketToTID[CONDVAR_MAX_NON_SPINWAIT_THREADS];
    int ticketCalled[CONDVAR_MAX_NON_SPINWAIT_THREADS];
    int nextTicketAvailable;
    int ticketOnCall;
} cond_t;

#endif /* _COND_TYPE_H */
