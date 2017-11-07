/** @file sem_type.h
 *  @brief This file defines the type for semaphores.
 */

#ifndef _SEM_TYPE_H
#define _SEM_TYPE_H

#include <cond.h>
#include <mutex.h>

// For outside caller you shouldn't care about these knobs. For detail please
// refer to sem.c
typedef struct sem {
    mutex_t mutex;
    cond_t cond;
    int resourceCount;
} sem_t;

#endif /* _SEM_TYPE_H */
