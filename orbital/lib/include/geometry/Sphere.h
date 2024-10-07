#pragma once

#include "../math/MathTypes.h"

namespace bfc {
  namespace geometry {
    template<typename T>
    class Sphere {
    public:
      Sphere()
        : Sphere({0, 0, 0}, 0) {}

      Sphere(Vector3<T> const & _center, T const & _radius)
        : center(_center)
        , radius(_radius) {}

      template<typename U>
      operator Sphere<U>() const {
        return Sphere<U>(Vector3<U>(_center), U(_radius));
      }

      T volume() const {
        return (4 * glm::pi<T>() * math::pow<3>(radius)) / 3;
      }

      bool contains(Sphere<T> const & sphere) const {
        return sphere.radius <= radius - glm::length(sphere.center - center);
      }

      bool contains(Vector3<T> const & point) const {
        Vector3<T> toPoint = point - center;
        return glm::dot(toPoint, toPoint) <= math::pow<2>(radius);
      }

      bool intersects(Sphere<T> const & sphere) const {
        Vector3<T> toSphere = sphere.center - center;
        return glm::dot(toSphere, toSphere) <= math::pow<2>(sphere.radius + radius);
      }

      bool intersects(Vector3<T> const & point) const {
        return contains(point);
      }

      void growToContain(Sphere<T> const & sphere) {
        T distToCenter = glm::length(sphere.center - center);
        radius         = math::max(distToCenter + sphere.radius, radius);
      }

      void growToContain(Vector3<T> const & point) {
        Vector3<T> toPoint = point - center;
        radius             = math::max(glm::length(toPoint), radius);
      }

      Vector3<T> center;
      T          radius;
    };

    using Spheref = Sphere<float>;
    using Sphered = Sphere<double>;
  } // namespace geometry

  /// Calculate the bounding sphere for T
  template<typename T>
  void calcBoundingSphere(T const & o) {
    static_assert(false, "calcBoundingSphere is not implemented for type T");
  }

  template<typename T>
  inline constexpr bool has_calc_bounding_sphere_v = !std::is_void_v<decltype(calcBoundingSphere(std::declval<T>()))>;
} // namespace bfc
