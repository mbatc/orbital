#pragma once

#include <limits>

#include "../math/MathTypes.h"

namespace bfc {
  namespace geometry {
    template<typename T>
    class Box {
    public:
      /// Default construct a box.
      /// The constructed box is invalid (has negative volume).
      Box() {
        min = Vector3<T>(std::numeric_limits<T>::max());
        max = Vector3<T>(-std::numeric_limits<T>::max());
      }

      Box(Vector3<T> const & _min, Vector3<T> const & _max) {
        min = _min;
        max = _max;
      }

      Box(Vector3<T> const & center, T const & size)
        : Box(center - size / 2, center + size / 2) {}

      template<typename U>
      operator Box<U>() const {
        return Box<U>(Vector3<U>(min), Vector3<U>(max));
      }

      Vector3<T> center() const {
        return (min + max) / T(2);
      }

      Vector3<T> size() const {
        return max - min;
      }

      Vector3<T> halfSize() const {
        return size() / T(2);
      }

      T volume() const {
        Vector3<T> dim = size();
        return dim.x * dim.y * dim.z;
      }

      T diagonal() const {
        return (T)math::length(size());
      }

      T longestEdge() const {
        return math::maxComponent(size());
      }

      T shortestEdge() const {
        return math::minComponent(size());
      }

      void growToContain(Box<T> const & box) {
        min = math::min(min, box.min);
        max = math::max(max, box.max);
      }

      void growToContain(Vector3<T> const & point) {
        min = math::min(min, point);
        max = math::max(max, point);
      }

      Box<T> projected(Vector3<T> right, Vector3<T> up, Vector3<T> forward) const {
        Vector3<T> oldCenter = center();
        Vector3<T> newCenter = {glm::dot(right, oldCenter), glm::dot(up, oldCenter), glm::dot(forward, oldCenter)};

        Vector3<T> extents   = halfSize();
        right *= extents.x;
        up *= extents.y;
        forward *= extents.z;

        Vector3<T> newDim = math::abs(right) + math::abs(up) + math::abs(forward);
        return Box<T>(newCenter - newDim, newCenter + newDim);
      }

      void transform(Matrix<T> const & m) {
        Vector3<T> newCenter = Vector3<T>(m * Vector4<T>(center(), 1));

        Vector3<T> extents   = halfSize();
        Vector3<T> right   = m[0] * extents.x;
        Vector3<T> up      = m[1] * extents.y;
        Vector3<T> forward = m[2] * extents.z;

        Vector3<T> newDim = math::abs(right) + math::abs(up) + math::abs(forward);

        min = newCenter - newDim;
        max = newCenter + newDim;
      }

      void translate(Vector3<T> const & offset) {
        min += offset;
        max += offset;
      }

      void scale(Vector3<T> const & scale) {
        min *= scale;
        max *= scale;
      }

      bool contains(Vector3<T> const & point) const {
        return point.x >= min.x && point.y >= min.y && point.z >= min.z && point.x <= max.x && point.y <= max.y && point.z <= max.z;
      }

      bool contains(Box<T> const & box) const {
        return box.min.x >= min.x && box.min.y >= min.y && box.min.z >= min.z && box.max.x <= max.x && box.max.y <= max.y && box.max.z <= max.z;
      }

      bool overlaps(Vector3<T> const & point) const {
        return contains(point);
      }

      bool overlaps(Box<T> const & box) const {
        return box.min.x <= max.x && box.min.y <= max.y && box.min.z <= max.z && box.max.x >= min.x && box.max.y >= min.y && box.max.z >= min.z;
      }

      Box<T> intersection(Box<T> const & box) const {
        return { math::max(min, box.min), math::min(max, box.max) };
      }

      bool invalid() const {
        return !math::isFinite(min) || !math::isFinite(max);
      }

      union {
        struct {
          Vector3<T> min;
          Vector3<T> max;
        };

        Vector3<T> extents[2];
      };
    };

    using Boxf = Box<float>;
    using Boxd = Box<double>;
  } // namespace geometry

  /// Calculate the bounding box for T
  template<typename T>
  void calcBoundingBox(T const & o) {
    static_assert(false, "calcBoundingBox is not implemented for type T");
  }

  template<typename T>
  inline constexpr bool has_calc_bounding_box_v = !std::is_void_v<decltype(calcBoundingBox(std::declval<T>()))>;
} // namespace bfc
