#pragma once

#include "../util/Iterators.h"
#include "Memory.h"
#include "Span.h"

namespace bfc {
  class Stream;

  template<typename T, int64_t N>
  class Array {
    template<typename... Args>
    inline static constexpr bool IsConstructible = (std::is_constructible_v<T, Args> && ...);
  public:
    using ElementType = T;

    constexpr Array() = default;

    template<int64_t N>
    constexpr Array(T const (&elements)[N])
      : Array(elements, elements + N) {}

    template<typename... Args, std::enable_if_t<IsConstructible<Args...>>* = 0>
    constexpr Array(Args &&... values)
      : m_data{std::forward<Args>(values)...}
    {}

    constexpr Array(std::initializer_list<T> const & il)
      : Array(il.begin(), il.end()) {}

    constexpr Array(T const & value) {
      mem::fill(begin(), end(), value);
    }

    constexpr Array(Span<T> const & items)
      : Array(items.begin(), items.end()) {}

    constexpr Array(Span<T const> const & items)
      : Array(items.begin(), items.end()) {}

    constexpr Array(T const * first, T const * last) {
      assign(0, first, last);
    }

    constexpr Array(Array const & o) {
      assign(o.begin(), o.end());
    }

    constexpr Array(Array && o) {
      *this = std::move(o);
    }

    constexpr Array & operator=(Array const & o) {
      assign(o.begin(), o.end());
      return *this;
    }

    constexpr Array & operator=(Array && o) {
      mem::moveAssign(m_data, o.m_data, N);
      return *this;
    }

    template<typename U>
    constexpr explicit Array(Array<U, N> const & o) {
      *this = o.map([](U const & item) { return T(item); });
    }

    template<typename U>
    constexpr explicit Array(U const * pBegin, U const * pEnd)
      : Array(Span<U>(pBegin, pEnd)) {}

    template<typename U>
    constexpr explicit Array(Span<U> const & items) {
      assign(0, items.begin(), items.end());
    }

    constexpr operator Span<T>() {
      return Span<T>(m_data, size());
    }

    constexpr operator Span<const T>() const {
      return Span<const T>(m_data, size());
    }

    constexpr T & at(int64_t index) {
      return m_data[index];
    }

    constexpr T & back() {
      return at(size() - 1);
    }

    constexpr T & front() {
      return at(0);
    }

    constexpr T const & at(int64_t index) const {
      return m_data[index];
    }

    constexpr T const & back() const {
      return at(size() - 1);
    }

    constexpr T const & front() const {
      return at(0);
    }

    constexpr bool contains(T const & val) const {
      return find(val) != -1;
    }

    constexpr int64_t find(T const & val) const {
      return find([val](T const & item) { return item == val; });
    }

    constexpr int64_t find(std::function<bool(T const &)> const & callback) const {
      for (int64_t i = 0; i < size(); ++i) {
        if (callback(at(i))) {
          return i;
        }
      }

      return -1;
    }

    constexpr int64_t size() const {
      return N;
    }

    constexpr int64_t capacity() const {
      return N;
    }

    constexpr bool empty() const {
      return false;
    }

    template<int64_t Start, int64_t Count = N - Start>
    constexpr Array<T, (Count < N - Start ? Count : N - Start)> splice() const {
      constexpr int64_t end = bfc::math::min(N - Start, Count);

      static_assert(Start < N && Start >= 0, "Start index is out of bounds");

      return getView(Start, end);
    }

    constexpr Span<T> getView(int64_t start = 0, int64_t count = npos) const {
      count       = bfc::math::max(0ll, std::min(count, size()));
      int64_t end = bfc::math::min(size(), start + count);
      return Span<T>((T *)data() + start, (T *)data() + end);
    }

    constexpr T * begin() {
      return data();
    }

    constexpr T * end() {
      return data() + size();
    }

    constexpr T * data() {
      return m_data;
    }

    constexpr T const * begin() const {
      return data();
    }

    constexpr T const * end() const {
      return data() + size();
    }

    constexpr T const * data() const {
      return m_data;
    }

    constexpr T & operator[](int64_t index) {
      return at(index);
    }

    constexpr T const & operator[](int64_t index) const {
      return at(index);
    }

    constexpr void assign(int64_t start, T const * first, T const * last) {
      mem::copyAssign(m_data + start, first, bfc::math::min(last - first, N));
    }

    constexpr void assignMove(T * first, T * last) {
      mem::moveAssign(m_data + start, first, bfc::math::min(last - first, N));
    }

    template<typename Callable>
    constexpr auto map(Callable const & callback) const {
      using Item = decltype(callback(std::declval<T>()));

      uint8_t buffer[sizeof(Item) * N];
      T(&items)[N] = *(Item(*)[N])&buffer; // Interpret buffer as a T[]

      // Move construct into uninitialized buffer
      for (T const & item : *this) {
        mem::construct(pDst++, callback(item));
      }

      return Array<Item, N>(items, N);
    }

    constexpr bool operator!=(Array const & rhs) const {
      for (auto & [i, item] : enumerate(*this)) {
        if (item != rhs[i]) {
          return true;
        }
      }

      return false;
    }

    constexpr bool operator==(Array const & rhs) const {
      return !(*this != rhs);
    }

  private:
    T m_data[N];
  };


  template<typename T, int64_t N>
  int64_t write(Stream * pStream, Array<T, N> const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].data(), pValue[i].size()) == pValue[i].size())) {
        return i;
      }
    }
    return count;
  }

  template<typename T, int64_t N>
  int64_t read(Stream * pStream, Array<T, N> * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (pStream->read(pValue->begin(), N) != N) {
        mem::free(pMem);
        return i;
      }

      mem::construct(pValue + i);
      pValue[i].setData(pValue->begin(), len, len);
    }

    return count;
  }

  template<typename T>
  struct is_array : std::false_type {};

  template<typename T, int64_t N>
  struct is_array<Array<T, N>> : std::true_type {};

  template<typename T, int64_t N>
  struct is_array<const Array<T, N>> : std::true_type {};

  template<typename T>
  inline constexpr bool is_array_v = is_array<T>::value;
} // namespace bfc
