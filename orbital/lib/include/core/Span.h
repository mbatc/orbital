#pragma once

#include "Core.h"

namespace bfc {
  namespace impl {
    template<typename T>
    struct SpanDetail {
      static constexpr int64_t Stride = sizeof(T);
    };

    template<>
    struct SpanDetail<void> {
      static constexpr int64_t Stride = 1;
    };
  }
  template <typename T>
  class Span {
  public:
    static constexpr int64_t Stride = impl::SpanDetail<T>::Stride;

    constexpr Span() {}

    template <int64_t N>
    constexpr Span(T const (&elements)[N])
        : Span(elements, N) {
    }

    constexpr Span(T const * pData, int64_t size)
        : m_pData((T*)pData), m_size(math::max(0ll, size)) {}

    constexpr Span(T const * first, T const * last)
        : Span(first, last - first) {}

    template<typename U, std::enable_if_t<std::is_convertible_v<T*, U*>>* = 0>
    operator Span<U>() {
      return Span<U>((U *)m_pData, m_size * Stride / Span<U>::Stride);
    }

    template<typename U>
    explicit operator Span<U>() {
      return Span<U>((U *)m_pData, m_size * Stride / Span<U>::Stride);
    }

    // template<typename U, std::enable_if_t<std::is_convertible_v<T*, U*>>* = 0>
    // operator Span<U>() {
    //   return Span<U>((U *)m_pData, m_size * sizeof(T) / sizeof(U));
    // }

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
