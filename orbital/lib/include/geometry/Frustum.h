#pragma once

#include "../math/MathTypes.h"
#include "Plane.h"

namespace bfc {
  namespace geometry {
    template<typename T>
    class Frustum {
    public:
      Frustum(Matrix<T> const & vp = glm::identity<Matrix<T>>()) {
        Vector3<T> points[8] = {
            {-1, -1, -1}, // 0: Near-bot-left
            {-1,  1, -1}, // 1: Near-top-left
            { 1,  1, -1}, // 2: Near-top-right
            { 1, -1, -1}, // 3: Near-bot-right
            {-1, -1, 1},  // 4: Far-bot-left
            {-1,  1, 1},  // 5: Far-top-left
            { 1,  1, 1},  // 6: Far-top-right
            { 1, -1, 1},  // 7: Far-bot-right
        };

        Matrix<T> invVP = glm::inverse(vp);

        for (auto& p : points) {
          Vector4<T> a = invVP * Vector4<T>(p, 1);
          p = Vector3<T>(a) / a.w;
        }

        left  = Plane<T>(points[4], points[0], points[1]);
        right = Plane<T>(points[6], points[2], points[3]);
        bottom = Plane<T>(points[7], points[3], points[0]);
        top    = Plane<T>(points[5], points[1], points[2]);
        front  = Plane<T>(points[2], points[1], points[0]);
        back   = Plane<T>(points[5], points[6], points[7]);
      }

      union {
        struct {
          Plane<T> left;
          Plane<T> right;
          Plane<T> top;
          Plane<T> bottom;
          Plane<T> front;
          Plane<T> back;
        };

        Plane<T> planes[6];
      };
    };
  }
}
