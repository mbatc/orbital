#pragma once

#include "geometry/Geometry.h"

namespace engine {
  class RenderableStorageBase {
  public:
    virtual ~RenderableStorageBase() = default;

    virtual void    clear()                                         = 0;
    virtual int64_t size() const                                    = 0;
    virtual bool    erase(int64_t const & index, int64_t count = 1) = 0;

    virtual bfc::geometry::Spheref calcBoundingSphere(int64_t const & start = 0, int64_t const & count = bfc::npos) const = 0;
    virtual bool                   hasCalcBoundingSphere() const                                                          = 0;

    virtual bfc::geometry::Boxf calcBoundingBox(int64_t const & start = 0, int64_t const & count = bfc::npos) const = 0;
    virtual bool                hasCalcBoundingBox() const                                                          = 0;
  };

  template<typename T>
  class RenderableStorage : public RenderableStorageBase {
  public:
    virtual void clear() override {
      items.clear();
    }

    virtual int64_t size() const override {
      return items.size();
    }

    virtual bool erase(int64_t const & index, int64_t count = 1) override {
      return items.erase(index, count);
    }

    virtual bfc::geometry::Spheref calcBoundingSphere(int64_t const & start = 0, int64_t const & count = npos) const override {
      if constexpr (bfc::has_calc_bounding_sphere_v<T>) {
        bfc::geometry::Spheref sphere;
        int64_t                end = start + bfc::math::min(count, size() - start);
        for (int64_t i = start; i < end; ++i) {
          sphere.growToContain(bfc::calcBoundingSphere(items[i]));
        }
        return sphere;
      } else {
        return bfc::geometry::Spheref();
      }
    }

    virtual bool hasCalcBoundingSphere() const override {
      return bfc::has_calc_bounding_sphere_v<T>;
    }

    virtual bfc::geometry::Boxf calcBoundingBox(int64_t const & start = 0, int64_t const & count = npos) const override {
      if constexpr (bfc::has_calc_bounding_box_v<T>) {
        bfc::geometry::Boxf bounds;
        int64_t        end = start + bfc::math::min(count, size() - start);
        for (int64_t i = start; i < end; ++i) {
          bounds.growToContain(bfc::calcBoundingBox(items[i]));
        }
        return bounds;
      } else {
        return bfc::geometry::Boxf();
      }
    }

    virtual bool hasCalcBoundingBox() const override {
      return bfc::has_calc_bounding_box_v<T>;
    }

    T & at(int64_t index) {
      return items.at(index);
    }

    T & back() {
      return items.back();
    }

    T & front() {
      return items.front();
    }

    T const & at(int64_t index) const {
      return items.at(index);
    }

    T const & back() const {
      return items.back();
    }

    T const & front() const {
      return items.front();
    }

    bool contains(T const & val) const {
      return items.contains(val);
    }

    int64_t find(T const & val) const {
      return items.find(val);
    }

    int64_t find(std::function<bool(T const &)> const & callback) const {
      return items.find(callback);
    }

    void pushBack(T && value) {
      return items.pushBack(std::move(value));
    }

    void pushBack(T const & value) {
      return items.pushBack(value);
    }

    void pushBack(T const * first, T const * last) {
      return items.pushBack(first, last);
    }

    void pushBack(bfc::Span<T> const & items) {
      return items.pushBack(items);
    }

    void pushBack(std::initializer_list<T> const & il) {
      return items.pushBack(il);
    }

    void pushBackMove(T * first, T * last) {
      return items.pushBackMove(first, last);
    }

    void pushFront(T && value) {
      return items.pushFront(std::move(value));
    }

    void pushFront(T const & value) {
      return items.pushFront(value);
    }

    void pushFront(T const * first, T const * last) {
      return items.pushFront(first, last);
    }

    void pushFront(bfc::Span<T> const & items) {
      return items.pushFront(items);
    }

    void pushFront(std::initializer_list<T> const & il) {
      return items.pushFront(il);
    }

    void pushFrontMove(T * first, T * last) {
      return items.pushFrontMove(first, last);
    }

    T popBack() {
      return items.popBack();
    }

    T popFront() {
      return items.popFront();
    }

    void insert(int64_t index, T && value) {
      return items.insert(index, std::move(value));
    }

    void insert(int64_t index, T const & value) {
      return items.insert(index, value);
    }

    void insert(int64_t index, bfc::Span<T> const & items) {
      return items.insert(index, items);
    }

    void insert(int64_t index, T const * first, T const * last) {
      return items.insert(index, first, last);
    }

    void insertMove(int64_t index, T * first, T * last) {
      return items.insertMove(index, first, last);
    }

    void resize(int64_t newSize, T const & value = T{}) {
      return items.resize(newSize, value);
    }

    bool reserve(int64_t newCapacity) {
      return items.reserve(newCapacity);
    }

    int64_t capacity() const {
      return items.capacity();
    }

    bool empty() const {
      return items.empty();
    }

    bfc::Vector<T> getElements(int64_t start, int64_t count) const {
      return items.getElements(start, count);
    }

    bfc::Span<T> getView(int64_t start = 0, int64_t count = npos) const {
      return items.getView(start, count);
    }

    T * begin() {
      return items.begin();
    }
    T * end() {
      return items.end();
    }
    T * data() {
      return items.data();
    }

    T const * begin() const {
      return items.begin();
    }
    T const * end() const {
      return items.end();
    }
    T const * data() const {
      return items.data();
    }

    T & operator[](int64_t index) {
      return items.at(index);
    }

    T const & operator[](int64_t index) const {
      return items.at(index);
    }

    template<typename Callable>
    auto map(Callable const & callback) const {
      return items.map(callback);
    }

  private:
    bfc::Vector<T> items;
  };
}
