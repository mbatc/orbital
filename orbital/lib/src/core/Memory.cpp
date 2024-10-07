#include "core/Memory.h"

namespace bfc
{
  namespace mem
  {
    void* alloc(int64_t size) {
      return ::std::malloc(size);
    }

    void *realloc(void *ptr, int64_t size) {
      return ::std::realloc(ptr, size);
    }

    void free(void *ptr) {
      ::std::free(ptr);
    }
  }
}
