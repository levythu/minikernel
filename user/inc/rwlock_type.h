/** @file rwlock_type.h
 *  @brief This file defines the type for reader/writer locks.
 */

#ifndef _RWLOCK_TYPE_H
#define _RWLOCK_TYPE_H

#include <mutex.h>
#include <cond.h>

// For outside caller you shouldn't care about these knobs. For detail please
// refer to rwlock.c
typedef struct rwlock {
  mutex_t mutex;
  cond_t alarm;

  int state;
  int nextAvailableWriterTicket;
  int nextAvailableReaderTicket;
  int writerTicketOnCall;
  int readerTicketOnCall;
} rwlock_t;

#endif /* _RWLOCK_TYPE_H */
