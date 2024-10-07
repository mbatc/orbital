#pragma once

#include "Core.h"

namespace bfc {
  template<typename Key, typename Value>
  class PackedMap {
  public:
    int64_t add(Key const & key, Value const& value) {
      int64_t index = m_values.size();
      if (!m_keyToIndex.tryAdd(key, index)) {
        return -1;
      }
      m_values.pushBack(value);
      return index;
    }

    int64_t set(Key const & key, Value const & value) {
      int64_t index = find(key);
      if (index != -1) {
        m_values[index] = value;
      } else {
        index = m_values.size();
        m_keyToIndex.add(key, index);
        m_values.pushBack(value);
      }
      return index;
    }

    Value & get(Key const & key) {
      return getAt(find(key));
    }

    Value getOr(Key const & key, Value const & defaultValue) const {
      Value const * pValue = tryGet(key);
      return pValue != nullptr ? *pValue : defaultValue;
    }

    Value & getAt(int64_t index) {
      return m_values[index];
    }

    Value * tryGet(Key const & key) {
      return tryGetAt(find(key));
    }

    Value * tryGetAt(int64_t index) {
      return index < 0 || index >= m_values.size() ? nullptr : m_values.begin() + index;
    }

    Value const & get(Key const & key) const {
      return getAt(find(key));
    }

    Value const & getAt(int64_t index) const {
      return m_values[index];
    }

    Value const * tryGet(Key const & key) const {
      return tryGetAt(find(key));
    }

    Value const * tryGetAt(int64_t index) const {
      return index < 0 || index >= m_values.size() ? nullptr : m_values.begin() + index;
    }

    int64_t find(Key const& key) const {
      return m_keyToIndex.getOr(key, -1);
    }

    bool contains(Key const& key) const {
      return m_keyToIndex.contains(key);
    }

    int64_t size() const {
      return m_values.size();
    }

    bool erase(Key const& key) {
      int64_t index = find(key);
      if (index == -1) {
        return false;
      }

      m_keyToIndex.erase(key);
      m_values.erase(index);
      return true;
    }

    Vector<Value> const& getValues() const {
      return m_values;
    }

    Vector<Key> getKeys() const {
      return m_keyToIndex.getKeys();
    }

  private:
    Map<Key, int64_t> m_keyToIndex;
    Vector<Value>     m_values;
  };
}
