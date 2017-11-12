/** @file source_untrusted.h
 *
 *  @brief Helper utilities for validate and get data from untrusted source
 *
 *  All function inside is non-thread safe nor interrupt safe. Generally the
 *  caller use memlock to protect thread safeness, while avoid calling it inside
 *  interrupt handler to allow interrupt unsafeness.
 *
 *  @author Leiyu Zhao
 */

#ifndef SOURCE_UNTRUSTED_H
#define SOURCE_UNTRUSTED_H

// Verify a user space range, with given previlage.
// Returns false if and only if there's any address in the range [start, end]
// that: 1. fall in kernel memory 2. is not writable while mustWritable=true
bool verifyUserSpaceAddr(
    uint32_t startAddr, uint32_t endAddr, bool mustWritable);

// get an integer from addr to target. return true if success
bool sGetInt(uint32_t addr, int* target);

// get a NUL-terminated string from addr to target, return its size
// return -1 on failure
// return non-neg value indicating the number of chars get (include the
// terminating zero). Must <= size
//    if return < size, there must be a terminating zero at target[return - 1]
int sGetString(uint32_t addr, char* target, int size);

// get a NUL-terminated uint array from addr to target, return its size
// return -1 on failure
// return non-neg value indicating the number of uint get (include the
// terminating zero). Must <= size
//    if return < size, there must be a terminating zero at target[return - 1]
int sGeUIntArray(uint32_t addr, uint32_t* target, int size);

#endif
