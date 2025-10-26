#include "core/Memory.h"
#include <cstring>

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

    char* strdup(const char* str) {
      return _strdup(str);
    }

    void strcpy(char * dst, size_t bufferSize, const char * src) {
      strcpy_s(dst, bufferSize, src);
    }
  }
}
