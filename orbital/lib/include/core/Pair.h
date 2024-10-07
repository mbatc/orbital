#pragma once

#include "Core.h"
#include <functional>

namespace bfc {
  class Stream;

  template<typename Key, typename Value>
  class Pair {
  public:
    inline constexpr Pair(Key && fst, Value && sec)
      : first(std::move(fst))
      , second(std::move(sec)) {}

    inline constexpr Pair(Key const & fst = Key{}, Value const & sec = Value{})
      : first(fst)
      , second(sec) {}

    inline constexpr bool operator==(Pair const & pair) const {
      return pair.first == first && pair.second == second;
    }

    inline constexpr bool operator!=(Pair const & pair) const {
      return !(this == pair);
    }

    Key   first;
    Value second;
  };

  template<typename Key, typename Value>
  inline constexpr Pair<Key, Value> makePair(Key key, Value value) {
    return { key, value };
  }

  template<typename Key, typename Value>
  int64_t write(Stream * pStream, Pair<Key, Value> const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].first) && pStream->write(pValue[i].second))) {
        return i;
      }
    }
    return count;
  }

  template<typename Key, typename Value>
  int64_t read(Stream * pStream, Pair<Key, Value> * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].first) && pStream->read(&pValue[i].second))) {
        return i;
      }
    }
    return count;
  }
} // namespace bfc

namespace std {
  template<typename Key, typename Value>
  struct hash<bfc::Pair<Key, Value>> {
    size_t operator()(const bfc::Pair<Key, Value> & o) const {
      return bfc::hash(o.first, o.second);
    }
  };
} // namespace std
