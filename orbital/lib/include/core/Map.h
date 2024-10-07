#pragma once

#include "Pair.h"
#include "Vector.h"

namespace bfc {
  template <typename Key, typename Value>
  class Map {
  public:
    using KeyType = Key;
    using ValueType = Value;

    using Item = Pair<Key, Value>;
    using Bucket = Vector<Item>;

    constexpr static int64_t BucketSize = 16ll;

    Map(int64_t capacity = 0)
        : m_bucketOrder(findBucketOrder(capacity)) {
      m_buckets.resize(getBucketCount());
    }

    Map(std::initializer_list<Item> const& items)
        : Map(items.size()) {
      for (Item const& i : items)
        tryAdd(i);
    }

    Map(Span<Item> const& items)
        : Map(items.size()) {
      for (Item const& i : items)
        tryAdd(i);
    }

    bool contains(Key const& key) const {
      return tryGet(key) != nullptr;
    }

    Value& add(Key&& key, Value&& value) {
      tryRehash();
      int64_t bucketIndex = findBucket(key);

      BFC_ASSERT(bucketIndex >= 0 && bucketIndex < m_buckets.size(), "Invalid Bucket");

      Bucket& bucket = m_buckets[bucketIndex];

      BFC_ASSERT(findInBucket(bucket, key) == -1, "Duplicate Key");

      bucket.pushBack(Item(std::move(key), std::move(value)));
      ++m_size;

      return bucket.back().second;
    }

    Value& add(Key const& key, Value const& value) {
      return add(std::move(Key(key)), std::move(Value(value)));
    }

    Value& add(Pair<Key, Value> const& pair) {
      return add(Key(pair.first), Value(pair.second));
    }

    Value& add(Pair<Key, Value>&& pair) {
      return add(std::move(pair.first), std::move(pair.second));
    }

    bool tryAdd(Key&& key, Value&& value) {
      tryRehash();

      Bucket& bucket = m_buckets[findBucket(key)];
      int64_t index = findInBucket(bucket, key);
      if (index != -1)
        return false;

      bucket.pushBack(Item(std::move(key), std::move(value)));
      ++m_size;
      return true;
    }

    bool tryAdd(Key const& key, Value const& value) {
      return tryAdd(std::move(Key(key)), std::move(Value(value)));
    }

    bool tryAdd(Pair<Key, Value> const& pair) {
      return tryAdd(std::move(Key(pair.first)), std::move(Value(pair.second)));
    }

    bool tryAdd(Pair<Key, Value>&& pair) {
      return tryAdd(std::move(pair.first), std::move(pair.second));
    }

    Value & addOrSet(Key const & key, Value const & value) {
      tryRehash();

      int64_t bucketIndex = findBucket(key);

      BFC_ASSERT(bucketIndex >= 0 && bucketIndex < m_buckets.size(), "Invalid Bucket");

      Bucket& bucket = m_buckets[bucketIndex];
      int64_t index = findInBucket(bucket, key);
      if (index != -1) {
        bucket[index].second = value;
        return bucket[index].second;
      }
      else {
        bucket.pushBack(Item(key, value));
        ++m_size;
        return bucket.back().second;
      }
    }

    bool erase(Key const & key) {
      int64_t bucketIndex = findBucket(key);

      BFC_ASSERT(bucketIndex >= 0 && bucketIndex < m_buckets.size(), "Invalid Bucket");

      Bucket & bucket = m_buckets[bucketIndex];
      int64_t  index  = findInBucket(bucket, key);
      if (index == -1)
        return false;

      bucket.erase(index);
      --m_size;
      return true;
    }

    bool erase(Key const & key, Value * pValue) {
      int64_t bucketIndex = findBucket(key);

      BFC_ASSERT(bucketIndex >= 0 && bucketIndex < m_buckets.size(), "Invalid Bucket");

      Bucket & bucket = m_buckets[bucketIndex];
      int64_t  index  = findInBucket(bucket, key);
      if (index == -1)
        return false;

      if (pValue != nullptr) {
        *pValue = std::move(bucket[index].second);
      }

      bucket.erase(index);
      --m_size;
      return true;
    }

    /// Try get a value, and copy it to pDst.
    /// @retval false `key` was not in the map.
    /// @retval true  The value at `key` was copied to pDst.
    /// @remarks If `key` was not found then `pDst` is not modified
    bool tryGet(Key const & key, Value * pDst) const {
      BFC_ASSERT(pDst != nullptr, "pDst cannot be nullptr");
      Value const * pValue = tryGet(key);
      if (pValue == nullptr) {
        return false;
      }
      *pDst = *pValue;
      return true;
    }

    Value* tryGet(Key const& key) {
      Bucket& bucket = m_buckets[findBucket(key)];
      int64_t index = findInBucket(bucket, key);
      return index == -1 ? nullptr : &bucket[index].second;
    }

    Value const* tryGet(Key const& key) const {
      Bucket const& bucket = m_buckets[findBucket(key)];
      int64_t index = findInBucket(bucket, key);
      return index == -1 ? nullptr : &bucket[index].second;
    }

    Value getOr(Key const& key, Value const& defaultValue) const {
      Bucket const & bucket = m_buckets[findBucket(key)];
      int64_t index = findInBucket(bucket, key);
      return index == -1 ? defaultValue : bucket[index].second;
    }

    Value & getOrAdd(Key const & key) {
      tryRehash();

      Bucket& bucket = m_buckets[findBucket(key)];
      int64_t index = findInBucket(bucket, key);

      if (index == -1) {
        index = bucket.size();
        bucket.pushBack(Item(key, Value()));
        ++m_size;
      }

      return bucket[index].second;
    }

    Value& get(Key const& key) {
      Value* pValue = tryGet(key);
      // Assert if null
      return *pValue;
    }

    Value const& get(Key const& key) const {
      Value* pValue = tryGet(key);
      // Assert if null
      return *pValue;
    }

    Value& operator[](Key const& key) {
      return getOrAdd(key);
    }

    Value const& operator[](Key const& key) const {
      return get(key);
    }

    int64_t capacity() const {
      return getBucketCount() * BucketSize;
    }

    int64_t size() const {
      return m_size;
    }

    void clear() {
      for (Bucket& bucket : m_buckets)
        bucket.clear();
      m_size = 0;
    }

    Vector<Key> getKeys() const {
      Vector<Key> keys;
      keys.reserve(m_size);
      for (Bucket const& bucket : m_buckets)
        for (Item const& item : bucket)
          keys.pushBack(item.first);
      return keys;
    }

    Vector<Value> getValues() const {
      Vector<Value> values;
      values.reserve(m_size);
      for (Bucket const& bucket : m_buckets)
        for (Item const& item : bucket)
          values.pushBack(item.second);
      return values;
    }

    Vector<Pair<Key, Value>> getItems() const {
      Vector<Pair<Key, Value>> values;
      values.reserve(m_size);
      for (Bucket const & bucket : m_buckets)
        for (Item const & item : bucket)
          values.pushBack(item);
      return values;
    }

    class Iterator {
    public:
      Iterator(Bucket* pBucket, Item* pItem, Bucket* pEndBucket)
          : m_pBucket(pBucket), m_pItem(pItem), m_pEndBucket(pEndBucket) {}

      bool operator==(Iterator const& o) const { return o.m_pBucket == m_pBucket && o.m_pItem == m_pItem; }
      bool operator!=(Iterator const& o) const { return !(*this == o); }
      Item* operator->() const { return m_pItem; }
      Item& operator*() const { return *m_pItem; }

      Iterator& operator++() {
        if (m_pBucket == nullptr)
          return *this;
        if (m_pItem == nullptr)
          return *this;

        // Goto next item
        ++m_pItem;

        // Goto next bucket if we don't have a valid item
        while (m_pItem != nullptr && m_pItem >= m_pBucket->end()) {
          ++m_pBucket;
          if (m_pBucket >= m_pEndBucket) {
            m_pItem = nullptr;
            m_pBucket = nullptr;
          }
          else {
            m_pItem = m_pBucket->begin();
          }
        }

        return *this;
      }

    private:
      Bucket* m_pBucket = nullptr;
      Item* m_pItem = nullptr;
      Bucket* m_pEndBucket = nullptr;
    };

    class ConstIterator : public Iterator {
    public:
      ConstIterator(Bucket const* pBucket, Item const* pItem, Bucket const* pEnd)
          : Iterator((Bucket*)pBucket, (Item*)pItem, (Bucket*)pEnd) {}

      bool operator==(ConstIterator const& o) const { return Iterator::operator==(o); }
      bool operator!=(ConstIterator const& o) const { return Iterator::operator!=(o); }
      Item const* operator->() const { return Iterator::operator->(); }
      Item const& operator*() const { return Iterator::operator*(); }
      ConstIterator& operator++() { return (ConstIterator&)Iterator::operator++(); }
    };

    Iterator begin() {
      if (size() == 0)
        return end();
      else
        return Iterator(m_buckets.begin(), m_buckets.front().begin(), m_buckets.end());
    }

    Iterator end() {
      return Iterator(nullptr, nullptr, m_buckets.end());
    }

    ConstIterator begin() const {
      if (size() == 0)
        return end();
      else
        return ConstIterator(m_buckets.begin(), m_buckets.front().begin(), m_buckets.end());
    }

    ConstIterator end() const {
      return ConstIterator(nullptr, nullptr, m_buckets.end());
    }

    bool operator==(Map const& rhs) const {
      if (size() != rhs.size()) {
        return false;
      }

      for (auto& [key, rhsVal] : rhs) {
        Value const * pValue = tryGet(key);
        if (pValue == nullptr || *pValue != rhsVal) {
          return false;
        }
      }

      return true;
    }

    bool operator!=(Map const & rhs) const {
      return !(*this == rhs);
    }

    template<typename Key2, typename Value2>
    friend int64_t write(Stream * pStream, Map<Key2, Value2> const * pValue, int64_t count);

    template<typename Key2, typename Value2>
    friend int64_t read(Stream * pStream, Map<Key2, Value2> * pValue, int64_t count);

  private:
    bool tryRehash() {
      if (m_size + 1 <= capacity())
        return false;

      // Create a new map with capcity for size + 1 items
      Map rehashed(m_size + 1);

      // Move all items to the new map
      auto it = begin();
      auto last = end();
      for (; it != last; ++it) {
        rehashed.add(std::move(it->first), std::move(it->second));
      }

      *this = std::move(rehashed);

      return true;
    }

    static int64_t findInBucket(Bucket const& bucket, Key const& key) {
      for (int64_t i = 0; i < bucket.size(); ++i)
        if (bucket[i].first == key)
          return i;
      return -1;
    }

    int64_t findBucket(Key const& key) const {
      return getBucketIndex(hash(key));
    }

    static constexpr int64_t findBucketOrder(int64_t capacity) {
      int64_t order = 0;
      while (order < 30 && getBucketCount(order) * BucketSize < capacity)
        ++order;
      return order;
    }

    uint64_t getBucketCount() const {
      uint64_t count = getBucketCount(m_bucketOrder);
      return count < 0 ? m_buckets.size() : count;
    }

    uint64_t getBucketIndex(uint64_t const& hashCode) const {
      switch (m_bucketOrder) {
      case 0:
        return hashCode % 1;
      case 1:
        return hashCode % 3;
      case 2:
        return hashCode % 7;
      case 3:
        return hashCode % 13;
      case 4:
        return hashCode % 29;
      case 5:
        return hashCode % 59;
      case 6:
        return hashCode % 127;
      case 7:
        return hashCode % 233;
      case 8:
        return hashCode % 547;
      case 9:
        return hashCode % 1153;
      case 10:
        return hashCode % 2137;
      case 11:
        return hashCode % 4391;
      case 12:
        return hashCode % 8191;
      case 13:
        return hashCode % 16931;
      case 14:
        return hashCode % 32467;
      case 15:
        return hashCode % 65449;
      case 16:
        return hashCode % 125149;
      case 17:
        return hashCode % 255329;
      case 18:
        return hashCode % 512747;
      case 19:
        return hashCode % 1045151;
      case 20:
        return hashCode % 2013751;
      case 21:
        return hashCode % 4002839;
      case 22:
        return hashCode % 8008549;
      case 23:
        return hashCode % 16007441;
      case 24:
        return hashCode % 32007299;
      case 25:
        return hashCode % 64010267;
      case 26:
        return hashCode % 128010517;
      default:
        return hashCode % m_buckets.size();
      }
    }

    static constexpr int64_t getBucketCount(int64_t order) {
      switch (order) {
      case 0:
        return 1;
      case 1:
        return 3;
      case 2:
        return 7;
      case 3:
        return 13;
      case 4:
        return 29;
      case 5:
        return 59;
      case 6:
        return 127;
      case 7:
        return 233;
      case 8:
        return 547;
      case 9:
        return 1153;
      case 10:
        return 2137;
      case 11:
        return 4391;
      case 12:
        return 8191;
      case 13:
        return 16931;
      case 14:
        return 32467;
      case 15:
        return 65449;
      case 16:
        return 125149;
      case 17:
        return 255329;
      case 18:
        return 255329;
      case 19:
        return 512747;
      case 20:
        return 512747;
      case 21:
        return 1045151;
      case 22:
        return 2013751;
      case 23:
        return 4002839;
      case 24:
        return 4002839;
      case 25:
        return 8008549;
      case 26:
        return 16007441;
      case 27:
        return 32007299;
      case 28:
        return 64010267;
      case 29:
        return 128010517;
      default:
        return -1;
      }
    }

    Vector<Bucket> m_buckets;
    int64_t m_bucketOrder = 0;
    int64_t m_size = 0;
  };

  template<typename Key, typename Value>
  int64_t write(Stream * pStream, Map<Key, Value> const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].m_buckets) && pStream->write(pValue[i].m_size) && pStream->write(pValue[i].m_bucketOrder))) {
        return i;
      }
    }
    return count;
  }

  template<typename Key, typename Value>
  int64_t read(Stream * pStream, Map<Key, Value> * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].m_buckets) && pStream->read(&pValue[i].m_size) && pStream->read(&pValue[i].m_bucketOrder))) {
        return i;
      }
    }
    return count;
  }

  template<typename T>
  struct is_map : std::false_type {};

  template<typename Key, typename Value>
  struct is_map<Map<Key, Value>> : std::true_type {};

  template<typename Key, typename Value>
  struct is_map<const Map<Key, Value>> : std::true_type {};

  template<typename T>
  inline constexpr bool is_map_v = is_map<T>::value;
}
