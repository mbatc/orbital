#pragma once

#include "../math/MathTypes.h"

namespace bfc {
  namespace geometry {
    template<typename T>
    class Plane {
    public:
      union {
        struct {
          T x;
          T y;
          T z;
          T w;
        };

        struct {
          Vector3<T> normal;
          T          distance;
        };
        Vector4<T> equation;
        T          a[4];
      };

      Plane() {
        normal   = Vector3<T>(0);
        distance = 0;
      }

      Plane(Vector4<T> const& _equation) {
        equation = _equation;
      }

      Plane(Vector3<T> const & a, Vector3<T> const & b, Vector3<T> const & c) {
        normal = glm::cross(b - a, b - c);
        normal = glm::normalize(normal);
        distance = glm::dot(normal, a);
      }

      Plane(Vector3<T> const & _normal, T const & _distance) {
        normal   = _normal;
        distance = _distance;
      }

      Plane(T _x, T _y, T _z, T _w) {
        x = _x;
        y = _y;
        z = _z;
        w = _w;
      }

      bool intersects(Vector3<T> const & position, Vector3<T> const & direction, T * pTime = nullptr) {
        // assuming vectors are all normalized
        T denom = glm::dot(normal, direction);
        if (denom < glm::epsilon<T>()) {
          return false;
        }

        if (pTime != nullptr) {
          *pTime = distanceTo(position) / denom;
        }
        return true;
      } 

      float distanceTo(Vector3<T> const & point) const {
        return glm::dot(normal, point) - w;
      }

      T & at(int64_t i) {
        return a[i];
      }

      T const & at(int64_t i) const {
        return a[i];
      }

      T & operator[](int64_t i) {
        return at(i);
      }

      T const & operator[](int64_t i) const {
        return at(i);
      }

      T * begin(int64_t i) {
        return a;
      }

      T const * begin(int64_t i) const {
        return a;
      }

      T * end(int64_t i) {
        return a + 4;
      }

      T const * end(int64_t i) const {
        return a + 4;
      }
    };
  } // namespace geometry
} // namespace bfc
