#pragma once

#include "Vector.h"

namespace bfc {
  template <typename T>
  class Pool {
  public:
    class Iterator {
    public:
      Iterator(int64_t startIndex, Pool* pPool)
          : m_pPool(pPool), m_index(startIndex) {}

      T& operator*() { return m_pPool->get(m_index); }
      bool operator==(Iterator const& o) const { return m_index == o.m_index && m_pPool == o.m_pPool; }
      bool operator!=(Iterator const& o) const { return !(*this == o); }

      Iterator& operator++() {
        do {
          ++m_index;
        } while (m_index < m_pPool->capacity() && !m_pPool->isUsed(m_index));
        return *this;
      }

    private:
      Pool* m_pPool = nullptr;
      int64_t m_index = 0;
    };

    class ConstIterator {
    public:
      ConstIterator(int64_t index, Pool const* pPool)
          : m_pPool(pPool), m_index(index) {}

      T const& operator*() { return m_pPool->get(m_index); }
      bool operator==(ConstIterator const& o) const { return m_index == o.m_index && m_pPool == o.m_pPool; }
      bool operator!=(ConstIterator const& o) const { return !(*this == o); }

      ConstIterator& operator++() {
        do {
          ++m_index;
        } while (m_index < m_pPool->capacity() && !m_pPool->isUsed(m_index));
        return *this;
      }

    private:
      Pool const* m_pPool = nullptr;
      int64_t m_index = 0;
    };

    Pool(int64_t initialCapacity = 0) {
      reserve(initialCapacity);
    }

    int64_t size() const {
      return m_size;
    }

    int64_t capacity() const {
      return m_capacity;
    }

    bool isUsed(int64_t index) const {
      return index >= 0 && index < capacity() && m_used[index];
    }

    int64_t insert(T const& value) {
      return emplace(value);
    }

    int64_t insert(T&& value) {
      return emplace(std::move(value));
    }

    template <typename... Args>
    int64_t emplace(Args&&... args) {
      int64_t index = m_size;
      if (m_freed.size() > 0) {
        index = m_freed.popBack();
      }

      int64_t requiredSize = index + 1;
      if (requiredSize > m_capacity) {
        reserve(requiredSize * 2);
      }
      mem::construct(m_pData + index, std::forward<Args>(args)...);
      m_used[index] = true;

      ++m_size;

      return index;
    }

    T& get(int64_t index) {
      return m_pData[index];
    }

    T const& get(int64_t index) const {
      return m_pData[index];
    }

    T& operator[](int64_t index) {
      return get(index);
    }

    T const& operator[](int64_t index) const {
      return get(index);
    }

    T* tryGet(int64_t index) {
      return isUsed(index) ? data() + index : nullptr;
    }

    T const* tryGet(int64_t index) const {
      return isUsed(index) ? data() + index : nullptr;
    }

    bool erase(int64_t index) {
      if (index < 0 || index >= m_used.size() || !m_used[index])
        return false;
      dubiousErase(index);
      return true;
    }

    void dubiousErase(int64_t index) {
      m_used[index] = false;
      m_freed.pushBack(index);
      mem::destruct(m_pData + index);
      --m_size;
    }

    bool reserve(int64_t newCapacity) {
      if (newCapacity <= m_capacity) {
        return false;
      }

      T* pNewBuffer = mem::alloc<T>(newCapacity);
      for (int64_t i = 0; i < m_capacity; ++i) {
        if (m_used[i]) {
          mem::moveConstruct(pNewBuffer + i, m_pData + i, 1);
          mem::destruct(m_pData + i);
        }
      }

      std::swap(pNewBuffer, m_pData);
      mem::free(pNewBuffer);
      m_capacity = newCapacity;
      m_used.resize(newCapacity, false);
      return true;
    }

    Iterator begin() { return Iterator(first(), this); }
    Iterator end() { return Iterator(last(), this); }

    ConstIterator begin() const { return ConstIterator(first(), this); }
    ConstIterator end() const { return ConstIterator(last(), this); }

    T const* data() const {
      return m_pData;
    }

    T* data() {
      return m_pData;
    }

    void clear() {
      for (auto& [i, used] : enumerate(m_used)) {
        if (used) {
          mem::destruct(m_pData + i);
          used = false;
        }
      }
      m_freed.clear();
      m_size = 0;
    }

    template<typename T>
    friend int64_t write(Stream * pStream, Pool<T> const * pValue, int64_t count);
    template<typename T>
    friend int64_t read(Stream * pStream, Pool<T> * pValue, int64_t count);

  private:
    int64_t first() const {
      if (m_size == 0)
        return 0;
      int64_t i = 0;
      while (!m_used[i])
        ++i;
      return i;
    }

    int64_t last() const {
      if (m_size == 0)
        return 0;

      return m_capacity;
    }

    int64_t m_size = 0;
    int64_t m_capacity = 0;

    T* m_pData = nullptr;

    Vector<int64_t> m_freed;
    Vector<bool> m_used;
  };

  template <typename T, typename RefType = int64_t>
  class RefPool {
  public:
    // Expose subset of Pool functions
    bool isUsed(int64_t index) const { return m_pool.isUsed(index); }
    int64_t capacity() const { return m_pool.capacity(); }
    int64_t size() const { return m_pool.size(); }

    T* data() { return m_pool.data(); }
    T& get(int64_t index) { return m_pool.get(index); }
    T* tryGet(int64_t index) { return m_pool.tryGet(index); }
    T& operator[](int64_t index) { return m_pool.get(index); }
    auto begin() { return m_pool.begin(); }
    auto end() { return m_pool.end(); }

    T const* data() const { return m_pool.data(); }
    T const& get(int64_t index) const { return m_pool.get(index); }
    T const& operator[](int64_t index) const { return m_pool.get(index); }
    T const* tryGet(int64_t index) const { return m_pool.tryGet(index); }
    auto begin() const { return m_pool.begin(); }
    auto end() const { return m_pool.end(); }

    RefPool(int64_t initialCapacity = 0) {
      reserve(initialCapacity);
    }

    int64_t insert(T const& value) {
      return emplace(value);
    }

    int64_t insert(T&& value) {
      return emplace(std::move(value));
    }

    template <typename... Args>
    int64_t emplace(Args&&... args) {
      int64_t index = m_pool.emplace(std::forward<Args>(args)...);
      m_references.resize(m_pool.capacity(), -1);
      m_references[index] = 1;
      return index;
    }

    bool erase(int64_t index) {
      if (!isUsed(index))
        return false;
      dubiousErase(index);
      return true;
    }

    void dubiousErase(int64_t index) {
      m_pool.dubiousErase(index);
      m_references[index] = 0;
    }

    bool reserve(int64_t newCapacity) {
      if (!m_pool.reserve(newCapacity))
        return false;
      m_references.resize(newCapacity, 0);
      return true;
    }

    // Reference counting API

    int64_t addRef(int64_t index) {
      if (!isUsed(index))
        return -1;
      return ++m_references[index];
    }

    int64_t release(T const& item) {
      return release(&item - data());
    }

    int64_t release(int64_t index) {
      if (!isUsed(index))
        return -1;
      return --m_references[index];
    }

    int64_t getReferenceCount(int64_t index) const {
      return isUsed(index) ? m_references[index] : -1;
    }

  private:
    Vector<RefType> m_references;
    Pool<T> m_pool;
  };

  template<typename T>
  int64_t write(Stream * pStream, Pool<T> const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].m_size) &&
        pStream->write(pValue[i].m_capacity) &&
        pStream->write(pValue[i].m_freed) &&
        pStream->write(pValue[i].m_used)))
        return i;

      for (int64_t j = 0; j < pValue[i].m_capacity; ++j) {
        if (pValue[i].m_used[j]) {
          if (!pStream->write(pValue[i].m_pData[j]))
            return i;
        }
      }
    }
    return count;
  }

  template<typename T>
  int64_t read(Stream * pStream, Pool<T> * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].m_size) &&
        pStream->read(&pValue[i].m_capacity) &&
        pStream->read(&pValue[i].m_freed) &&
        pStream->read(&pValue[i].m_used)))
        return i;

      T *pData = mem::alloc<T>(pValue[i].m_capacity);
      for (int64_t j = 0; j < pValue[i].m_capacity; ++j) {
        if (pValue[i].m_used[j]) {
          if (!pStream->read(&pData[j])) {
            mem::free(pData);
            return i;
          }
        }
      }

      pValue[i].m_pData = pData;
    }
    return count;
  }
}
