#pragma once

#include "Pool.h"
#include "Vector.h"

namespace bfc {
  template<typename T>
  class Set {
  public:
    inline static const double GrowRate        = std::sqrt(2);
    inline static const double InitialCapacity = 16;

    Set(int64_t capacity = InitialCapacity) {
      m_lookup.resize(std::max(1ll, capacity), Unused);
      m_items.reserve(capacity);
    }

    Set(Span<T> const & items)
      : Set(items.begin(), items.end()) {}

    Set(T * pBegin, T * pEnd) {
      while (pBegin < pEnd) {
        add(*(pBegin++));
      }
    }

    /// Get the number of items in the set.
    int64_t size() const {
      return m_items.size();
    }

    /// Get the number of items that can be stored before needing to reallocate (and rehash).
    int64_t capacity() const {
      return m_lookup.size();
    }

    /// Check if an item is in the set
    bool contains(T const & item) const {
      return m_items.isUsed(find(item));
    }

    /// Add an item to the set.
    bool add(T const & item) {
      int64_t lookupIndex = findLookupIndex(item);
      if (lookupIndex < 0 || m_lookup[lookupIndex] >= 0) {
        return false;
      }

      m_lookup[lookupIndex] = m_items.insert(item);
      tryRehash();
      return true;
    }

    /// Remove an item from the set.
    /// @retval true  The item was removed.
    /// @retval false The item was not found in the set.
    bool erase(T const & item) {
      int64_t lookupIndex = findLookupIndex(item);
      if (lookupIndex < 0 || m_lookup[lookupIndex] < 0) {
        return false;
      }

      int64_t itemIndex = m_lookup[lookupIndex];
      m_items.erase(itemIndex);
      m_lookup[lookupIndex] = Tombstone;
      return true;
    }

    Vector<T> keys() const {
      Vector<T> ret;
      ret.reserve(size());
      for (T const & item : *this)
        ret.pushBack(item);
      return ret;
    }

    auto begin() {
      return m_items.begin();
    }

    auto end() {
      return m_items.end();
    }

    auto begin() const {
      return m_items.begin();
    }

    auto end() const {
      return m_items.end();
    }

  private:
    bool tryRehash() {
      if (size() + 1 < capacity()) {
        return false;
      }

      int64_t newSize = bfc::math::max(1ll, int64_t(size() * GrowRate));
      // Create a new map with capacity for size + 1 items
      Set rehashed(newSize);
      // Move all items to the new map
      auto it   = begin();
      auto last = end();
      for (; it != last; ++it) {
        rehashed.add(std::move(*it));
      }

      *this = std::move(rehashed);
      return true;
    }

    int64_t find(T const & item) const {
      int64_t idx = findLookupIndex(item);
      return idx < 0 ? -1 : m_lookup[idx];
    }

    int64_t findLookupIndex(T const & item) const {
      uint64_t itemHash    = hash(item);
      int64_t  lookupIndex = itemHash % m_lookup.size();
      int64_t  count       = 0;
      while (count < m_lookup.size()) {
        int64_t itemIndex = m_lookup[lookupIndex];
        if (itemIndex == Unused) {
          break;
        }

        if (m_items[itemIndex] == item) {
          return lookupIndex;
        }

        lookupIndex = (lookupIndex + 1) % m_lookup.size();
        ++count;
      }

      return count == m_lookup.size() ? -1 : lookupIndex;
    }

    constexpr inline static int64_t Unused    = -1;
    constexpr inline static int64_t Tombstone = -2;

    Vector<int64_t> m_lookup;
    Pool<T>         m_items;
  };
} // namespace bfc
