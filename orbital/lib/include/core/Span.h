#pragma once

#include "Core.h"

namespace bfc {
  template <typename T>
  class Span {
  public:
    constexpr Span() {}

    template <int64_t N>
    constexpr Span(T const (&elements)[N])
        : Span(elements, N) {
    }

    constexpr Span(T const * pData, int64_t size)
        : m_pData((T*)pData), m_size(size) {}

    constexpr Span(T const * first, T const * last)
        : Span(first, last - first) {}

    operator Span<const T>() { return Span<const T>(m_pData, m_size); }

    constexpr int64_t size() const { return m_size; }
    constexpr T& front() const { return *begin(); }
    constexpr T& back() const { return *(end() - 1); }
    constexpr T& at(int64_t index) const { return m_pData[index]; }
    constexpr T* begin() const { return m_pData; }
    constexpr T* end() const { return m_pData + m_size; }
    constexpr T* data() const { return m_pData; }

    constexpr Span getElements(int64_t start, int64_t count = npos) const {
      count = std::min(count, size() - start);
      if (count < 0)
        return Span();
      return Span(begin() + start, count);
    }

    constexpr T& operator[](int64_t index) const { return at(index); }

  private:
    T*      m_pData = nullptr;
    int64_t m_size  = 0;
  };
}
