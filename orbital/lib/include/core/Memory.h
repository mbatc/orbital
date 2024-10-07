#pragma once

#include "Core.h"
#include <memory>

namespace bfc
{
  namespace mem
  {
    BFC_API void *alloc(int64_t size);

    BFC_API void *realloc(void *ptr, int64_t size);

    BFC_API void free(void *ptr);

    template<typename T>
    constexpr T copy(T const & o) {
      return o;
    }

    template<typename T>
    T* alloc(int64_t count = 1) {
      return (T*)alloc(count * sizeof(T));
    }

    template<typename T, typename... Args>
    constexpr void construct(T *dst, Args&&... args) {
      new (dst) T(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    constexpr void constructArray(T * dst, int64_t count, Args &&... args) {
      while (count-- > 0) {
        construct(dst++, copy(args)...);
      }
    }

    template<typename T>
    constexpr void copyAssign(T & dst, T const & src) {
      dst = src;
    }

    template<typename T>
    constexpr void moveAssign(T & dst, T & src) {
      dst = std::move(src);
    }

    template<typename T>
    constexpr void copyConstruct(T * dst, T const * src, int64_t count) {
      // if constexpr (std::is_trivially_copy_constructible_v<T>) {
      //   memcpy(dst, src, sizeof(T) * count);
      // }
      // else {
      //   while (count-- > 0)
      //     construct(dst++, *(src++));
      // }
      while (count-- > 0)
        construct(dst++, *(src++));
    }

    template<typename T>
    constexpr void copyAssign(T * dst, T const * src, int64_t count) {
      // if constexpr (std::is_trivially_copy_assignable_v<T>) {
      //   memcpy(dst, src, sizeof(T) * count);
      // }
      // else {
      //   while (count-- > 0)
      //     copyAssign(*(dst++), *(src++));
      // }
      while (count-- > 0)
        copyAssign(*(dst++), *(src++));
    }

    template<typename T>
    void copyConstructOrAssign(T *dst, T const * src, int64_t count) {
      if constexpr (std::is_copy_constructible_v<T>) {
        copyConstruct(dst, src, count);
      }
      else if constexpr (std::is_copy_assignable_v<T>) {
        copyAssign(dst, src, count);
      }
    }

    template<typename T>
    constexpr void moveConstruct(T * dst, T * src, int64_t count) {
      // if constexpr (std::is_trivially_move_constructible_v<T>) {
      //   memcpy(dst, src, sizeof(T) * count);
      // }
      // else {
      //   while (count-- > 0)
      //     construct(dst++, std::move(*(src++)));
      // }

      while (count-- > 0)
        construct(dst++, std::move(*(src++)));
    }

    template<typename T>
    constexpr void moveAssign(T *dst, T *src, int64_t count) {
      // if constexpr (std::is_trivially_move_assignable_v<T>) {
      //   memcpy(dst, src, sizeof(T) * count);
      // }
      // else {
      //   while (count-- > 0)
      //     moveAssign(*(dst++), *(src++));
      // }
      while (count-- > 0)
        moveAssign(*(dst++), *(src++));
    }

    template<typename T>
    constexpr void moveConstructOrAssign(T *dst, T *src, int64_t count) {
      if constexpr (std::is_move_constructible_v<T>) {
        moveConstruct(dst, src, count);
      }
      else if constexpr (std::is_move_assignable_v<T>) {
        moveAssign(dst, src, count);
      }
    }

    template<typename T>
    constexpr void destruct(T * dst, int64_t count = 1) {
      while (count-- > 0)
        (dst++)->~T();
    }
  }
}
