#include <stddef.h>
#include <malloc.h>
#include <malloc/malloc_internal.h>
#include <stdint.h>
#include <simics.h>

#include "cpu.h"

static CrossCPULock latch;
static uint32_t kernelMemAlloc;

void initMemManagement() {
  initCrossCPULock(&latch);
  kernelMemAlloc = 0;
}

/* safe versions of malloc functions */
void *malloc(size_t size) {
  GlobalLockR(&latch);
  void* ret = _malloc(size);
  GlobalUnlockR(&latch);
  return ret;
}

void *memalign(size_t alignment, size_t size) {
  GlobalLockR(&latch);
  void* ret = _memalign(alignment, size);
  GlobalUnlockR(&latch);
  return ret;
}

void *calloc(size_t nelt, size_t eltsize) {
  GlobalLockR(&latch);
  void* ret = _calloc(nelt, eltsize);
  GlobalUnlockR(&latch);
  return ret;
}

void *realloc(void *buf, size_t new_size) {
  GlobalLockR(&latch);
  void* ret = _realloc(buf, new_size);
  GlobalUnlockR(&latch);
  return ret;
}

void free(void *buf) {
  GlobalLockR(&latch);
  _free(buf);
  GlobalUnlockR(&latch);
}

void *smalloc(size_t size) {
  GlobalLockR(&latch);
  void* ret = _smalloc(size);
  if (ret) kernelMemAlloc += size;
  GlobalUnlockR(&latch);
  return ret;
}

void *smemalign(size_t alignment, size_t size) {
  GlobalLockR(&latch);
  void* ret = _smemalign(alignment, size);
  if (ret) kernelMemAlloc += size;
  GlobalUnlockR(&latch);
  return ret;
}

void sfree(void *buf, size_t size) {
  GlobalLockR(&latch);
  _sfree(buf, size);
  kernelMemAlloc -= size;
  GlobalUnlockR(&latch);
}

void reportKernelMemAlloc() {
  GlobalLockR(&latch);
  lprintf("├ Kernel Memory Allocator");
  lprintf("│ └ Bytes allocated: %lu", kernelMemAlloc);
  GlobalUnlockR(&latch);
}
