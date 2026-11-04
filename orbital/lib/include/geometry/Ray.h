#pragma once

#include "../math/MathTypes.h"

namespace bfc {
  namespace geometry {
    template<typename T>
    class Ray {
    public:
      bfc::Vector3<T> start;
      bfc::Vector3<T> direction;

      Ray(bfc::Vector3<T> const & start, bfc::Vector3<T> const & direction)
        : start(start)
        , direction(direction) {}

      static Ray line(bfc::Vector3<T> const& start, bfc::Vector3<T> const& end) {
        return {start, end - start};
      }

      static Ray normalized(bfc::Vector3<T> const& start, bfc::Vector3<T> const& direction) {
        return { start, bfc::math::normalize(direction) };
      }

      bfc::Vector3<T> at(T distance) const {
        return start + direction * distance;
      }

      Ray normalized() const {
        return Ray<T>::normalized(start, direction);
      }

      double length() const {
        return bfc::math::length(direction);
      }

      bfc::Vector3<T> end() const {
        return start + direction;
      }
    };

    using Rayf = Ray<float>;
    using Rayd = Ray<double>;
  }
} // namespace bfc
