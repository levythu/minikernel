/**
 *  @file autostack.c
 *
 *  @brief The library associated with runtime maintenance for memory stack
 *
 *  AutoStack works in two different modes, and they are different behavior:
 *
 *  Single Threaded Mode is the initial mode for every new process. In this mode
 *  AutoStack will catch segmentation fault, and try to *grow* the current stack
 *  to hold that address, i.e., allocate all the spaces from the current stack
 *  boundary to that address (aligned). If for any reason the growth cannot be
 *  performed, the error message is printed and program exits with -2. In single
 *  mode the program can use switchToMultiThreadMode to switch to multithreaded
 *  mode.
 *
 *  Multi-Threaded Mode must be active before thread lib is initiated (actaully
 *  the switch is doen automatically by thread lib) to create any new thread.
 *  In this mode, every thread will have a fix sized memory stack (called thread
 *  stack block) and each thread stack block is located with some interval so
 *  that some invalid memory access can be caught by segfault. Meanwhile, in
 *  this mode stack block will not grow anymore, instead, the process panics.
 *
 *  All available thread stack blocks are stored in createdThreadBlock, and
 *  sequential search is performed to get an available block. If there's nothing
 *  available, a new block of memory is allocated.
 *
 *  On transition to multithreaded mode, the current stack block is converted to
 *  one of used thread stack blocks, while it may be of different size, and it's
 *  okay.
 *
 *  @author Leiyu Zhao
 *
 */

#include <stdint.h>
#include <simics.h>
#include <syscall.h>
#include <stddef.h>
#include <stdlib.h>

#include <autostack_thread.h>

static uint32_t currentStackHigh;
static uint32_t currentStackLow;

static uint8_t handlerStack[PAGE_SIZE];

/******************************************************************************/
/* Multi Threaded auto stack */
// Note: multithread related autostack functions are not thread-safe. It must be
// protected against thread lib's mutexes

static threadStackBlock createdThreadBlock[THREAD_NUM_LIMIT];
static int numThreadBlock;
static uint32_t lowestAddressOfThreadStackBlock;
static int threadStackSize_;

void deallocateThreadBlock(threadStackBlock* block) {
  block->present = false;
}

threadStackBlock* allocateThreadBlock() {
  for (int i = 0; i < numThreadBlock; i++) {
    if (!createdThreadBlock[i].present) {
      createdThreadBlock[i].present = true;
      return &createdThreadBlock[i];
    }
  }
  // Allocate gap
  uint32_t gapStart = lowestAddressOfThreadStackBlock - PAGE_SIZE;
  if (new_pages(
      (void*)gapStart, lowestAddressOfThreadStackBlock - gapStart) < 0) {
    lprintf(
      "Failed to claim all space from 0x%08lx to 0x%08lx.",
      gapStart,
      lowestAddressOfThreadStackBlock
    );
    return NULL;
  }
  lowestAddressOfThreadStackBlock = gapStart;

  // Check whether thread-limit exceeds
  if (numThreadBlock == THREAD_NUM_LIMIT) {
    lprintf("Thread Limit Exeeded!");
    return NULL;
  }

  // Allocate new threadBlock
  uint32_t newBlockLow = lowestAddressOfThreadStackBlock - threadStackSize_;
  if (new_pages(
      (void*)newBlockLow, lowestAddressOfThreadStackBlock - newBlockLow) < 0) {
    lprintf(
      "Failed to claim all space from 0x%08lx to 0x%08lx.",
      newBlockLow,
      lowestAddressOfThreadStackBlock
    );
    return NULL;
  }
  createdThreadBlock[numThreadBlock].present = true;
  createdThreadBlock[numThreadBlock].low = newBlockLow;
  createdThreadBlock[numThreadBlock].high = lowestAddressOfThreadStackBlock;
  numThreadBlock++;
  lowestAddressOfThreadStackBlock = newBlockLow;
  lprintf(
    "Allocating new stack block: [0x%08lx, 0x%08lx)",
    createdThreadBlock[numThreadBlock-1].low,
    createdThreadBlock[numThreadBlock-1].high
  );
  return &createdThreadBlock[numThreadBlock-1];
}

static void multiThreadHandler(void *arg, ureg_t *ureg) {
  lprintf(
    "Caught exception: cause = %u, cr2 = 0x%08x",
    ureg->cause,
    ureg->cr2
  );
  if (ureg->cause != SWEXN_CAUSE_PAGEFAULT) {
    // Non-pagefault, leave it as it is. Terminate the program
    panic("Process exits due to non-caught exception.");
  }
  panic("Process exits due to PAGEFAULT.");
}

// Install the multiThreadHandler on current thread, return whether it succeed
// stackBase is the highest address of stack (inclusive)
bool installMultithreadHandler(uint32_t stackBase) {
  if (swexn((void*)stackBase, multiThreadHandler, NULL, NULL) < 0) {
    return false;
  }
  return true;
}

// threadStackSize must be page-aligned
int switchToMultiThreadMode(
    uint32_t threadStackSize, threadStackBlock** initialBlock) {
  // First of all, increase the current stack size to threadStackSize
  lprintf("Autostack switching to fixed stack size mode, size = %08lx",
      threadStackSize);
  if (currentStackHigh - currentStackLow + 1 < threadStackSize) {
    uint32_t targetStackLow = currentStackHigh - threadStackSize + 1;
    if (new_pages(
        (void*)targetStackLow, currentStackLow - targetStackLow) < 0) {
      lprintf("Fail to enlarge current stack with space [0x%08lx, 0x%08lx)",
          targetStackLow, currentStackLow);
      return FAIL_TO_ENLARGE_CURRENT_THREAD;
    }
    lprintf(
      "Enlarge the current stack to [0x%08lx, 0x%08lx]",
      targetStackLow,
      currentStackHigh
    );
    currentStackLow = targetStackLow;
  }
  // Then convert it
  numThreadBlock = 1;
  lowestAddressOfThreadStackBlock = currentStackLow;
  createdThreadBlock[0].low = currentStackLow;
  // Even if it overflows, it's okay
  createdThreadBlock[0].high = currentStackHigh + 1;
  createdThreadBlock[0].present = true;
  threadStackSize_ = threadStackSize;
  *initialBlock = &createdThreadBlock[0];

  // And register the singleThreadHandler. Then we are good to go!
  // Note that all multithreaded handlers runs on the bottom of own stack,
  // since the handler will panic directly, there's no need to preserve
  // current stack anymore
  if (!installMultithreadHandler(currentStackHigh)) {
    return SYSCALL_FAILURE;
  }
  return 0;
}

/******************************************************************************/
/* Single Threaded auto stack */

static void singleThreadHandler(void *arg, ureg_t *ureg) {
  lprintf(
    "Caught exception: cause = %u, cr2 = 0x%08x",
    ureg->cause,
    ureg->cr2
  );
  if (ureg->cause != SWEXN_CAUSE_PAGEFAULT) {
    // Non-pagefault, leave it as it is. Terminate the program
    vanish();
  }
  if (ureg->cr2 > currentStackHigh) {
    // Oh we cannot help with this!
    lprintf(
      "Program is trying to access address 0x%08x, which is above the stack",
      ureg->cr2
    );
    vanish();
  }
  if (ureg->cr2 < currentStackLow) {
    // try to claim all the space
    uint32_t alignedStart = ALIGN_PAGE(ureg->cr2);
    if (new_pages((void*)alignedStart, currentStackLow - alignedStart) < 0) {
      lprintf(
        "Failed to claim all space from 0x%08lx to 0x%08lx.",
        alignedStart,
        currentStackLow
      );
      vanish();
    }
    currentStackLow = alignedStart;
    lprintf(
      "Adjusted stack lowerbound to 0x%08lx",
      currentStackLow
    );
  }
  swexn(handlerStack + PAGE_SIZE, singleThreadHandler, NULL, ureg);
}

// This function is executing in single threaded mode (happens in _main)
// so we don't worry about its thread safety
void install_autostack(void* stack_high, void* stack_low) {
  currentStackHigh = (uint32_t)stack_high;
  currentStackLow = (uint32_t)stack_low;
  lprintf(
    "Initiating auto stack library, stack range [0x%08lx, 0x%08lx]",
    currentStackLow, currentStackHigh
  );

  // What if now we are stepping out of stack limit? (before setup handler)
  // Um... let it crash!

  lprintf("Registering the handler, result = %d",
    swexn(handlerStack + PAGE_SIZE, singleThreadHandler, NULL, NULL));
}
