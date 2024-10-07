#pragma once

#include "../math/MathTypes.h"

namespace bfc {
  namespace geometry {
    template<typename T>
    class Rectangle {
    public:
      /// Default construct a box.
      /// The constructed box is invalid (has negative volume).
      Rectangle() {
        min = Vector2<T>(std::numeric_limits<T>::max());
        max = Vector2<T>(-std::numeric_limits<T>::min());
      }

      Rectangle(Vector2<T> const & _min, Vector2<T> const & _max) {
        min = _min;
        max = _max;
      }

      Rectangle(Vector2<T> const & center, T const & size)
        : Box(center - size / 2, center + size / 2) {}

      Vector2<T> center() const {
        return (min + max) / 2;
      }

      Vector2<T> size() const {
        return max - min;
      }

      Vector2<T> halfSize() const {
        return size() / 2;
      }

      T area() const {
        Vector2<T> dim = size();
        return dim.x * dim.y;
      }

      T diagonal() const {
        return (T)glm::length(size());
      }

      T longestEdge() const {
        return math::maxComponent(size());
      }

      T shortestEdge() const {
        return math::minComponent(size());
      }

      void growToContain(Box<T> const & box) {
        min = math::min(box.min);
        max = math::max(box.max);
      }

      void growToContain(Vector2<T> const & point) {
        min = math::min(point);
        max = math::max(point);
      }

      bool contains(Vector2<T> const & point) const {
        return point.x >= min.x && point.y >= min.y && point.x <= max.x && point.y <= max.y;
      }

      bool contains(Box<T> const & box) const {
        return box.min.x >= min.x && box.min.y >= min.y && box.max.x <= max.x && box.max.y <= max.y;
      }

      bool overlaps(Vector2<T> const & point) const {
        return contains(point);
      }

      bool overlaps(Box<T> const & point) const {
        return box.min.x <= max.x && box.min.y <= max.y && box.max.x >= min.x && box.max.y >= min.y;
      }

      union {
        struct {
          Vector2<T> min;
          Vector2<T> max;
        };

        Vector2<T> extents[2];
      };
    };
    
    using Rectanglef = Rectangle<float>;
    using Rectangled = Rectangle<double>;

    /// Calculate the bounding rectangle for T
    template<typename T>
    void calcBoundingRectangle(T const& o) {
      static_assert(false, "calcBoundingRectangle is not implemented for type T");
    }

    template<typename T>
    using has_calc_bounding_rectangle = std::is_void_v<!decltype(calcBoundingRectangle(std::declval<T>()))>;
  }
}
