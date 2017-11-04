/** @file autostack_thread.h
 *  @brief the header file defining the values and functions used in AutoStack
 */

#ifndef AUTOSTACK_PRIVATE_H
#define AUTOSTACK_PRIVATE_H

#include <stdbool.h>

// The maximum number of thread cannot exceed this number
#define THREAD_NUM_LIMIT 64

#define THREAD_STACK_SIZE_TOO_SMALL -10
#define THREAD_LIMIT_REACHED -11
#define SYSCALL_FAILURE -12
#define FAIL_TO_ENLARGE_CURRENT_THREAD -13

// Get the largest aligned address that is <= addr
#define ALIGN_PAGE(addr) ((addr) / PAGE_SIZE * PAGE_SIZE)

// Get the lowest aligned address that is >= addr
#define ALIGN_PAGE_CEIL(addr) (((addr) + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE)

// Stack memory definition. Where [low, high) is the mem range of that stack and
// present is the flag meaning this memory is currently used by some non-dead
// thread
typedef struct threadStackBlock {
    uint32_t low;
    uint32_t high;
    bool present;
} threadStackBlock;

int switchToMultiThreadMode(
    uint32_t threadStackSize, threadStackBlock** initialBlock);
threadStackBlock* allocateThreadBlock();
void deallocateThreadBlock(threadStackBlock* block);
bool installMultithreadHandler(uint32_t stackBase);

#endif /* AUTOSTACK_PRIVATE_H */
