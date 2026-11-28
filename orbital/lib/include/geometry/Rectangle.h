#pragma once

#include "../math/MathTypes.h"
#include "../core/Serialize.h"

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
        : Rectangle(center - size / 2, center + size / 2) {}
      
      static Rectangle one() {
        return {{0, 0}, {1, 1}};
      }

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

      void growToContain(Rectangle<T> const & box) {
        min = math::min(min, box.min);
        max = math::max(max, box.max);
      }

      void growToContain(Vector2<T> const & point) {
        min = math::min(min, point);
        max = math::max(max, point);
      }

      bool contains(Vector2<T> const & point) const {
        return point.x >= min.x && point.y >= min.y && point.x <= max.x && point.y <= max.y;
      }

      bool contains(Rectangle<T> const & box) const {
        return box.min.x >= min.x && box.min.y >= min.y && box.max.x <= max.x && box.max.y <= max.y;
      }

      bool overlaps(Vector2<T> const & point) const {
        return contains(point);
      }

      bool overlaps(Rectangle<T> const & box) const {
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
    inline static constexpr bool has_calc_bounding_rectangle = !std::is_void_v<decltype(calcBoundingRectangle(std::declval<T>()))>;
  }
  
  template<typename T>
  struct Serialization<geometry::Rectangle<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, geometry::Rectangle<T> const & o) {
      return SerializedObject::MakeMap({{"min", ar(o.min)}, {"max", ar(o.max)}});
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, geometry::Rectangle<T> & o) {
      ar(s.get("min"), ReadOrConstruct(o.min));
      ar(s.get("max"), ReadOrConstruct(o.max));
      return true;
    }
  };
}
