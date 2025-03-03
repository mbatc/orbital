#pragma once

#include "Memory.h"
#include "Span.h"
#include "../util/Iterators.h"

namespace bfc {
  class Stream;

  template <typename T>
  class Vector {
  public:
    using ElementType = T;

    Vector() = default;

    template<int64_t N>
    Vector(T const (&elements)[N])
      : Vector(elements, elements + N) {}

    Vector(std::initializer_list<T> const & il)
      : Vector(il.begin(), il.end()) {}

    Vector(int64_t const & initialSize, T const & value) {
      resize(initialSize, value);
    }

    Vector(Span<T> const & items)
      : Vector(items.begin(), items.end()) {}

    Vector(Span<T const> const & items)
      : Vector(items.begin(), items.end()) {}

    Vector(T const * first, T const * last) {
      pushBack(first, last);
    }

    Vector(Vector const & o) {
      assign(o.begin(), o.end());
    }

    Vector(Vector && o) {
      *this = std::move(o);
    }

    Vector & operator=(Vector const & o) {
      assign(o.begin(), o.end());
      return *this;
    }

    Vector & operator=(Vector && o) {
      std::swap(m_pData, o.m_pData);
      std::swap(m_size, o.m_size);
      std::swap(m_capacity, o.m_capacity);
      return *this;
    }

    template<typename U>
    explicit Vector(Vector<U> const & o) {
      *this = std::move(o.map([](U const & item) { return T(item); }));
    }

    template<typename U>
    explicit Vector(U const * pBegin, U const * pEnd)
      : Vector(Span<U>(pBegin, pEnd)) {}

    template<typename U>
    explicit Vector(Span<U> const & items) {
      reserve(items.size());
      for (U const & item : items)
        pushBack(T(item));
    }

    ~Vector() {
      clear();
      mem::free(m_pData);
      m_capacity = 0;
      m_pData    = nullptr;
    }

    operator Span<T>() {
      return Span<T>(m_pData, size());
    }
    operator Span<const T>() const {
      return Span<const T>(m_pData, size());
    }

    T & at(int64_t index) {
      return m_pData[index];
    }
    T & back() {
      return at(size() - 1);
    }
    T & front() {
      return at(0);
    }

    T const & at(int64_t index) const {
      return m_pData[index];
    }
    T const & back() const {
      return at(size() - 1);
    }
    T const & front() const {
      return at(0);
    }

    bool contains(T const & val) const {
      return find(val) != -1;
    }

    int64_t find(T const & val, int64_t start = 0) const {
      return find([val](T const & item) { return item == val; }, start);
    }

    int64_t find(std::function<bool(T const &)> const & callback, int64_t start = 0) const {
      for (int64_t i = math::max(start, 0ll); i < size(); ++i) {
        if (callback(at(i))) {
          return i;
        }
      }

      return -1;
    }

    int64_t findReverse(T const & val, int64_t start = bfc::npos) const {
      return findReverse([val](T const & item) { return item == val; }, start);
    }

    int64_t findReverse(std::function<bool(T const &)> const & callback, int64_t start = bfc::npos) const {
      for (int64_t i = math::min(start, size()); i >= 0; --i) {
        if (callback(at(i))) {
          return i;
        }
      }

      return -1;
    }

    void pushBack(T && value) {
      insert(size(), std::move(value));
    }

    void pushBack(T const & value) {
      insert(size(), value);
    }

    void pushBack(T const * first, T const * last) {
      insert(size(), first, last);
    }

    void pushBack(Span<T> const & items) {
      insert(size(), items);
    }

    template<int64_t N>
    void pushBack(T const(&elements)[N]) {
      insert(size(), elements, elements + N);
    }

    void pushBack(std::initializer_list<T> const& il) {
      insert(size(), il);
    }

    void pushBackMove(T* first, T* last) {
      insertMove(size(), first, last);
    }

    void pushFront(T&& value) {
      insert(0, std::move(value));
    }

    void pushFront(T const& value) {
      insert(0, value);
    }

    void pushFront(T const* first, T const* last) {
      insert(0, first, last);
    }

    template<int64_t N>
    void pushFront(T const (&elements)[N]) {
      insert(0, elements);
    }

    void pushFront(Span<T> const& items) {
      insert(0, items);
    }

    void pushFront(std::initializer_list<T> const& il) {
      insert(0, il);
    }

    void pushFrontMove(T* first, T* last) {
      insert(0, first, last);
    }

    T popBack() {
      T value = std::move(back());
      erase(size() - 1);
      return std::move(value);
    }

    T popFront() {
      T value = std::move(front());
      erase(0);
      return std::move(value);
    }

    template<int64_t N>
    void insert(int64_t index, T const (&elements)[N]) {
      insert(index, elements, elements + N);
    }

    void insert(int64_t index, T&& value) {
      insertMove(index, &value, &value + 1);
    }

    void insert(int64_t index, T const& value) {
      insert(index, &value, &value + 1);
    }

    void insert(int64_t index, Span<T> const& items) {
      insert(index, items.begin(), items.end());
    }

    void insert(int64_t index, std::initializer_list<T> const& il) {
      insert(index, il.begin(), il.end());
    }

    void insert(int64_t index, T const * first, T const * last) {
      int64_t count = math::max(0ll, last - first);
      makeSpace(index, count);
      mem::copyConstruct(data() + index, first, count);
    }

    void insertMove(int64_t index, T * first, T * last) {
      int64_t count = math::max(0ll, last - first);
      makeSpace(index, count);
      mem::moveConstruct(data() + index, first, count);
    }

    bool erase(int64_t index, int64_t count = 1) {
      if (index == -1) {
        return false;
      }

      T* pStart = m_pData + index;
      T* pEnd = pStart + count;
      mem::destruct(pStart, count);
      mem::moveConstruct(pStart, pEnd, end() - pEnd);
      m_size -= count;

      return true;
    }

    bool eraseValue(T const & value) {
      return erase(find(value));
    }

    template<typename Pred>
    bool eraseIf(Pred&& callback) {
      bool removed = false;
      for (int64_t i = size() - 1; i >= 0; --i) {
        if (callback(at(i))) {
          removed |= erase(i);
        }
      }
      return removed;
    }

    void clear() {
      mem::destruct(m_pData, m_size);
      m_size = 0;
    }

    void resize(int64_t newSize, T const& value = T{}) {
      int64_t start = size();
      int64_t count = newSize - start;
      if (count > 0) {
        makeSpace(start, count);
        for (int64_t i = start; i < newSize; ++i)
          mem::construct(data() + i, value);
      }
      else {
        mem::destruct(end() + count, -count);
      }

      m_size = newSize;
    }

    bool reserve(int64_t newCapacity) {
      if (newCapacity < m_capacity)
        return false;
      return reallocate(newCapacity);
    }

    bool shrinkToFit() {
      return reallocate(m_size);
    }

    int64_t size() const {
      return m_size;
    }

    int64_t capacity() const {
      return m_capacity;
    }

    bool empty() const {
      return size() == 0;
    }

    Vector<T> getElements(int64_t start, int64_t count) const {
      return Vector<T>(getView(start, count));
    }

    Span<T> getView(int64_t start = 0, int64_t count = npos) const {
      count       = std::max(0ll, std::min(count, size()));
      int64_t end = std::min(size(), start + count);
      return Span<T>((T *)data() + start, (T *)data() + end);
    }

    T* begin() { return data(); }
    T* end() { return data() + size(); }
    T* data() { return m_pData; }

    T const* begin() const { return data(); }
    T const* end() const { return data() + size(); }
    T const* data() const { return m_pData; }

    T* takeData(int64_t* pSize = nullptr, int64_t* pCapacity = nullptr) {
      T* pData = m_pData;
      if (pSize == nullptr)
        *pSize = m_size;
      if (pCapacity == nullptr)
        *pCapacity = m_capacity;

      m_pData = nullptr;
      m_capacity = 0;
      m_size = 0;
      return pData;
    }

    void setData(T* pData, int64_t size, int64_t capcity = -1) {
      clear();
      shrinkToFit();
      m_pData = pData;
      m_size = size;
      m_capacity = capcity < 0 ? size : capcity;
    }

    T& operator[](int64_t index) {
      return at(index);
    }

    T const& operator[](int64_t index) const {
      return at(index);
    }

    void assign(T const* first, T const* last) {
      clear();
      pushBack(first, last);
    }

    void assignMove(T* first, T* last) {
      clear();
      pushBackMove(first, last);
    }

    template<typename Callable>
    auto map(Callable const & callback) const {
      using Item = decltype(callback(std::declval<T>()));

      Vector<Item> ret;
      ret.reserve(size());
      for (T const& item : *this) {
        ret.pushBack(callback(item));
      }
      return ret;
    }

    template<typename Callable>
    auto map(Callable const & callback) {
      using Item = decltype(callback(std::declval<T>()));

      Vector<Item> ret;
      ret.reserve(size());
      for (T & item : *this) {
        ret.pushBack(callback(item));
      }
      return ret;
    }

    bool operator!=(Vector const &rhs) const {
      if (size() != rhs.size()) {
        return false;
      }

      for (auto& [i, item] : enumerate(*this)) {
        if (item != rhs[i]) {
          return true;
        }
      }

      return false;
    }

    bool operator==(Vector const & rhs) const {
      return !(*this != rhs);
    }

  private:
    bool tryReserve(int64_t newCapacity) {
      if (m_capacity < newCapacity) {
        return reallocate(newCapacity)
      }
    }

    bool reallocate(int64_t newCapacity) {
      if (newCapacity == m_capacity)
        return false; // Don't reallocate - already enough space

      T* pNewBuffer = mem::alloc<T>(newCapacity);
      mem::moveConstruct(pNewBuffer, m_pData, m_size);
      std::swap(pNewBuffer, m_pData);
      std::free(pNewBuffer);
      m_capacity = newCapacity;
      return true;
    }

    void makeSpace(int64_t index, int64_t count) {
      int64_t requiredSpace = size() + count;
      // Ensure buffer is large enough
      if (requiredSpace > capacity()) {
        reallocate(requiredSpace * 2);
      }

      // Move elements after index
      T* pDst = data() + size() + count - 1;
      T* pSrc = pDst - count;
      int64_t numElements = size() - index;
      for (int64_t i = 0; i < numElements; ++i) {
        mem::moveConstruct(pDst - i, pSrc - i, 1);
        mem::destruct(pSrc - i);
      }

      m_size += count;
    }

    T* m_pData = nullptr;
    int64_t m_size = 0;
    int64_t m_capacity = 0;
  };

  template<typename Iterable, typename Callable>
  auto map(Iterable const & iterable, Callable const & callback) {
    using Item = decltype(callback(*iterable.begin()));

    Vector<Item> ret;
    ret.reserve(iterable.size());
    for (auto const & item : iterable) {
      ret.pushBack(callback(item));
    }

    return ret;
  }

  template<typename T>
  int64_t write(Stream * pStream, Vector<T> const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write((int64_t)pValue[i].size()) && pStream->write(pValue[i].data(), pValue[i].size()) == pValue[i].size())) {
        return i;
      }
    }
    return count;
  }

  template<typename T>
  int64_t read(Stream * pStream, Vector<T> * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      int64_t len = 0;
      if (!pStream->read(&len)) {
        return i;
      }

      T * pMem = mem::alloc<T>(len);
      if (pMem == nullptr || pStream->read(pMem, len) != len) {
        mem::free(pMem);
        return i;
      }

      mem::construct(pValue + i);
      pValue[i].setData(pMem, len, len);
    }

    return count;
  }

  template<typename T>
  struct is_vector : std::false_type {};

  template<typename T>
  struct is_vector<Vector<T>> : std::true_type {};

  template<typename T>
  struct is_vector<const Vector<T>> : std::true_type {};

  template<typename T>
  inline constexpr bool is_vector_v = is_vector<T>::value;
}
