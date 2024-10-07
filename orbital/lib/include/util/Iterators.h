#pragma once

#include "../core/Core.h"
#include "../core/Pair.h"

namespace bfc {
  template<typename T>
  struct Iterable {
    T itrBegin;
    T itrEnd;

    constexpr T const& begin() const {
      return itrBegin;
    }
    constexpr T const& end() const {
      return itrEnd;
    }
  };

  template<typename It>
  class EnumerateIterator {
  public:
    constexpr EnumerateIterator(It it)
        : iterator(it), index(0) {}

    using InnerItem = decltype(*std::declval<It>());
    using ItemRef = Pair<int64_t, InnerItem>;

    constexpr bool operator==(EnumerateIterator const& o) const { return iterator == o.iterator; }
    constexpr bool operator!=(EnumerateIterator const& o) const { return iterator != o.iterator; }

    constexpr ItemRef operator*() const { return { index, *iterator }; }

    constexpr EnumerateIterator& operator++() {
      ++iterator;
      ++index;
      return *this;
    }

    It iterator;
    int64_t index = 0;
  };

  template<typename It>
  constexpr Iterable<EnumerateIterator<It>> enumerate(It begin, It end) {
    return { EnumerateIterator<It>(begin), EnumerateIterator<It>(end) };
  }

  template <typename It>
  auto enumerate(It &item) {
    return enumerate(item.begin(), item.end());
  }

  template <typename It>
  auto enumerate(It const & item) {
    return enumerate(item.begin(), item.end());
  }

  template <typename It, int64_t N>
  auto enumerate(It const (&arr)[N]) {
    return enumerate(arr, arr + N);
  }

  template <typename It, int64_t N>
  auto enumerate(It (&arr)[N]) {
    return enumerate(arr, arr + N);
  }
}
