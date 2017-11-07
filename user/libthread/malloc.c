/*
 * these functions should be thread safe.
 * It is up to you to rewrite them
 * to make them thread safe.
 *
 */

#include <stdlib.h>
#include <types.h>
#include <stddef.h>
#include <mutex.h>
#include <stdbool.h>

static mutex_t allocProtector;
static bool isMultithread = false;

void initMultithread() {
  isMultithread = true;
  mutex_init(&allocProtector);
}

void *malloc(size_t __size)
{
  if (isMultithread) mutex_lock(&allocProtector);
  void* ret = _malloc(__size);
  if (isMultithread) mutex_unlock(&allocProtector);
  return ret;
}

void *calloc(size_t __nelt, size_t __eltsize)
{
  if (isMultithread) mutex_lock(&allocProtector);
  void* ret = _calloc(__nelt, __eltsize);
  if (isMultithread) mutex_unlock(&allocProtector);
  return ret;
}

void *realloc(void *__buf, size_t __new_size)
{
  if (isMultithread) mutex_lock(&allocProtector);
  void* ret = _realloc(__buf, __new_size);
  if (isMultithread) mutex_unlock(&allocProtector);
  return ret;
}

void free(void *__buf)
{
  if (isMultithread) mutex_lock(&allocProtector);
  _free(__buf);
  if (isMultithread) mutex_unlock(&allocProtector);
}
