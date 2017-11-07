/** @file thread_create.h
 *
 *  @brief The function signiture of the core asm code that wrap thread_fork
 *  and initate stack for the new thread
 */

#ifndef THREAD_CREATE_H
#define THREAD_CREATE_H

typedef void *(*ThreadPayload)(void *);
typedef void (*ThreadCreateCallback)(ThreadPayload, void*);

// Create a new thread on the stack, putting everything well
// stackTop: the top address (exclusive) of the stack where the new thread will
//           grow from
// callback: the function that will run in the new thread's context. NOTE:
//           callback MUST NOT return, the return address is a dead address!
// func/args: arguments that will be passed to callback function as it is.
// The function will return, with the TID of new thread, or a negative value,
// which means nothing is created
int thread_create(
    uint32_t stackTop,
    ThreadCreateCallback callback,
    ThreadPayload func,
    void* args);

#endif
