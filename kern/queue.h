/** @file queue.h
 *
 *  @brief A templated circular queue
 *
 *  varQueue is just the header of a general-type queue, with no payload. When
 *  you run MAKE_VAR_QUEUE_UTILITY(TYPE) in a .c file, it will generate
 *  several static functions for manipulating a varQueue:
 *
 *  void varQueueInit(varQueue* q, int capacity) will initiate the queue, with
 *  max capacity = capacity
 *
 *  void varQueueDestroy(varQueue* q) destroy a varQueueInit'd queue and release
 *  memory
 *
 *  bool varQueueEnq(varQueue* q, TYPE payload) enqueue a payload of given type,
 *  will fail if the queue has been filled. Return value indicates whether succ
 *
 *  TYPE varQueueDeq(varQueue* q) dequeue. It panics when there's nothing in the
 *  queue, so the caller should use q->size to check whether it's empty before
 *  calling this
 *
 *  Note that this data structure has no protection against multithread or
 *  interrupt at all. The caller should know that.
 *
 *  @author Leiyu Zhao
 */

#ifndef QUEUE_H
#define QUEUE_H

#include "bool.h"

typedef struct {
  void* payload;
  int size;
  int capacity;

  int head;
  int tail;
} varQueue;

#define MAKE_VAR_QUEUE_UTILITY(T) \
  static void __attribute__((unused)) varQueueInit(varQueue* q, int capacity) { \
    q->size = 0; \
    q->head = 0; \
    q->tail = 0; \
    q->capacity = capacity + 1; \
    q->payload = smalloc(sizeof(T) * q->capacity); \
    if (!q->payload) panic("Fail to init a queue."); \
  } \
 \
  static void __attribute__((unused)) varQueueDestroy(varQueue* q) { \
    sfree(q->payload, sizeof(T) * q->capacity); \
  } \
 \
  static bool __attribute__((unused)) varQueueEnq(varQueue* q, T payload){ \
    if ((q->head + 1) % q->capacity == q->tail) return false; \
    q->head = (q->head + 1) % q->capacity; \
    ((T*)q->payload)[q->head] = payload; \
    q->size++; \
    assert(q->size < q->capacity); \
    return true; \
  } \
 \
  static T __attribute__((unused)) varQueueDeq(varQueue* q) { \
    assert(q->size > 0); \
    q->tail = (q->tail + 1) % q->capacity; \
    T res = ((T*)q->payload)[q->tail]; \
    q->size--; \
    return res; \
  }

#endif
