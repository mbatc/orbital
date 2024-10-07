#pragma once

#include "Box.h"
#include "Frustum.h"
#include "Plane.h"
#include "Sphere.h"

namespace bfc {
  namespace geometry {
    template<typename T>
    bool classify(Frustum<T> const & a, Plane<T> const & b);

    /// Classify geometry against a plane.
    /// @returns less than 0 if the geometry is behind the plane.
    /// @returns more than 0 if the geometry is infront of the plane.
    /// @returns 0 if the geometry is on the plane.
    template<typename T>
    T classify(Box<T> const & a, Plane<T> const & b) {
      T boxSizeProjected = glm::dot(a.halfSize(), glm::abs(b.normal));
      T boxDistToPlane   = glm::dot(a.center(), b.normal) - b.distance;

      if (glm::abs(boxDistToPlane) < boxSizeProjected) {
        return T(0);
      } else if (boxDistToPlane < 0) {
        return boxDistToPlane + boxSizeProjected;
      } else {
        return boxDistToPlane - boxSizeProjected;
      }
    }

    template<typename T>
    T classify(Sphere<T> const & a, Plane<T> const & b) {
      T sphereDistToPlane = glm::dot(a.center, b.normal) + b.distance;

      if (glm::abs(sphereDistToPlane) < a.radius) {
        return T(0);
      } else if (sphereDistToPlane < 0) {
        return sphereDistToPlane + a.radius;
      } else {
        return sphereDistToPlane - a.radius;
      }
    }


    /// Test if a geometric primitive intersects with another geometric primitive.
    template<typename T>
    bool intersects(Frustum<T> const & a, Frustum<T> const & b);

    template<typename T>
    bool intersects(Frustum<T> const& a, Box<T> const& b) {
      for (Plane<T> const& plane : a.planes) {
        if (classify(b, plane) < 0) {
          return false;
        }
      }
      return true;
    }

    template<typename T>
    bool intersects(Frustum<T> const & a, Sphere<T> const & b) {
      for (Plane<T> const & plane : a.planes) {
        if (classify(b, plane) < 0) {
          return false;
        }
      }
      return true;
    }

    template<typename T>
    bool intersects(Box<T> const& a, Box<T> const& b) {
      return a.overlaps(b);
    }

    template<typename T>
    bool intersects(Box<T> const & a, Sphere<T> const & b) {
      Vector3<T> clamped = glm::clamp(b.center, a.min, a.max);
      return maxComponent(glm::abs(clamped - b.center)) <= b.radius;
    }

    template<typename T>
    bool intersects(Sphere<T> const & a, Sphere<T> const & b) {
      return a.intersects(b);
    }

    template<typename T>
    bool intersects(Sphere<T> const & a, Plane<T> const & b) {
      return classify(a, b) == T(0);
    }

    template<typename T>
    bool intersects(Plane<T> const & a, Plane<T> const & b) {
      return glm::abs(glm::dot(a.normal, b.normal)) - T(1) >= glm::epsilon<T>();
    }

    /// Test if a geometric primitive contains another geometric primitive.
    template<typename T>
    bool contains(Frustum<T> const & a, Frustum<T> const & b);
    template<typename T>
    bool contains(Frustum<T> const & a, Box<T> const & b);
    template<typename T>
    bool contains(Frustum<T> const & a, Sphere<T> const & b);
    template<typename T>
    bool contains(Frustum<T> const & a, Plane<T> const & b);

    template<typename T>
    bool contains(Box<T> const & a, Box<T> const & b);
    template<typename T>
    bool contains(Box<T> const & a, Sphere<T> const & b);
    template<typename T>
    bool contains(Box<T> const & a, Plane<T> const & b);

    template<typename T>
    bool contains(Sphere<T> const & a, Sphere<T> const & b);
    template<typename T>
    bool contains(Sphere<T> const & a, Plane<T> const & b);

    template<typename T>
    bool contains(Plane<T> const & a, Plane<T> const & b);
  } // namespace geometry

  template<typename T>
  geometry::Box<T> calcBoundingBox(geometry::Sphere<T> const & o) {
    return geometry::Box<T>(o.center, o.radius * 2);
  }

  template<typename T>
  geometry::Sphere<T> calcBoundingSphere(geometry::Box<T> const & o) {
    return geometry::Sphere<T>(o.center(), o.diagonal() / 2);
  }

  // clang-format off
  template<typename T>
  struct Reflect<geometry::Box<T>> {
    static inline constexpr auto get() {
      return makeReflection<geometry::Box<T>>(
        BFC_REFLECT(geometry::Box<T>, min),
        BFC_REFLECT(geometry::Box<T>, max),
        BFC_REFLECT(geometry::Box<T>, center), 
        BFC_REFLECT(geometry::Box<T>, size),
        BFC_REFLECT(geometry::Box<T>, halfSize),
        BFC_REFLECT(geometry::Box<T>, projected),
        BFC_REFLECT(geometry::Box<T>, transform),
        BFC_REFLECT(geometry::Box<T>, volume),
        BFC_REFLECT(geometry::Box<T>, diagonal),
        BFC_REFLECT(geometry::Box<T>, longestEdge),
        BFC_REFLECT(geometry::Box<T>, shortestEdge)
      );
    }
  };
  
  // clang-format on
} // namespace bfc
