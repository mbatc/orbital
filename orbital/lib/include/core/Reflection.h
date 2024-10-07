#pragma once

#include "Reflect.h"
#include "Span.h"
#include "StringView.h"

// clang-format off

namespace bfc {
  template<typename T>
  struct Reflect<Vector<T>> {
    static inline constexpr auto get() {
      return makeReflection<Vector<T>>(
        BFC_REFLECT_OVERLOAD(Vector<T>, begin, T*, ()),
        BFC_REFLECT_OVERLOAD(Vector<T>, end, T*, ()),
        BFC_REFLECT_OVERLOAD(Vector<T>, data,  T*, ()),
        BFC_REFLECT_OVERLOAD(Vector<T>, at, T&, (int64_t)),
        BFC_REFLECT_OVERLOAD(Vector<T>, back, T&, ()),
        BFC_REFLECT_OVERLOAD(Vector<T>, front, T&, ()),
        BFC_REFLECT_OVERLOAD(Vector<T>, pushBack, void, (T const &)),
        BFC_REFLECT_OVERLOAD(Vector<T>, pushFront, void, (T const &)),
        BFC_REFLECT_OVERLOAD(Vector<T>, insert, void, (int64_t, T const &)),
        BFC_REFLECT(Vector<T>, erase),
        BFC_REFLECT(Vector<T>, popFront),
        BFC_REFLECT(Vector<T>, popBack),
        BFC_REFLECT(Vector<T>, size),
        BFC_REFLECT(Vector<T>, clear)
      );
    }
  };

  template<typename Key, typename Value>
  struct Reflect<Map<Key, Value>> {
    static inline constexpr auto get() {
      using MapT = Map<Key, Value>;
      return makeReflection<MapT>(
        BFC_REFLECT_OVERLOAD(MapT, add, Value&, (Key const &, Value const &)),
        BFC_REFLECT_OVERLOAD(MapT, tryGet, Value*, (Key const &) const),
        BFC_REFLECT_OVERLOAD(MapT, getOrAdd, Value&, (Key const &, Value const &)),
        BFC_REFLECT_OVERLOAD(MapT, addOrSet, Value&, (Key const &, Value const &)),
        BFC_REFLECT_OVERLOAD(MapT, getOr, Value, (Key const &, Value const &) const),
        BFC_REFLECT_OVERLOAD(MapT, tryAdd, bool, (Key const &, Value const &)),
        BFC_REFLECT_OVERLOAD(MapT, get, bool, (Key const &) const),
        BFC_REFLECT(MapT, capacity),
        BFC_REFLECT(MapT, size),
        BFC_REFLECT(MapT, contains),
        BFC_REFLECT(MapT, getItems),
        BFC_REFLECT(MapT, getKeys),
        BFC_REFLECT(MapT, getValues),
      );
    }
  };

  template<typename Key, typename Value>
  struct Reflect<Pair<Key, Value>> {
    static inline constexpr auto get() {
      return makeReflection<Pair<Key, Value>>(
        using PairT = Pair<Key, Value>;
        BFC_REFLECT(PairT, first),
        BFC_REFLECT(PairT, second),
      );
    }
  };
}

// clang-format on
