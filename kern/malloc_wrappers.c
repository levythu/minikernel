#include <stddef.h>
#include <malloc.h>
#include <malloc/malloc_internal.h>

/* safe versions of malloc functions */
void *malloc(size_t size)
{
  return _malloc(size);
}

void *memalign(size_t alignment, size_t size)
{
  return _memalign(alignment, size);
}

void *calloc(size_t nelt, size_t eltsize)
{
  return _calloc(nelt, eltsize);
}

void *realloc(void *buf, size_t new_size)
{
  return _realloc(buf, new_size);
}

void free(void *buf)
{
  return _free(buf);
}

void *smalloc(size_t size)
{
  return _smalloc(size);
}

void *smemalign(size_t alignment, size_t size)
{
  return _smemalign(alignment, size);
}

void sfree(void *buf, size_t size)
{
  return _sfree(buf, size);
}
