#pragma once

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "../../../vendor/glm/glm/detail/qualifier.hpp"
#include "../../../vendor/glm/glm/ext.hpp"
#include "../../../vendor/glm/glm/gtx/matrix_decompose.hpp"
#include "../../../vendor/glm/glm/mat4x4.hpp"
#include "../../../vendor/glm/glm/vec2.hpp"
#include "../../../vendor/glm/glm/vec3.hpp"
#include "../../../vendor/glm/glm/vec4.hpp"
#include "../core/Reflect.h"

namespace bfc {
  template<int64_t N, typename T>
  using VectorN = glm::vec<N, T>;
  template<typename T>
  using Vector2 = VectorN<2, T>;
  template<typename T>
  using Vector3 = VectorN<3, T>;
  template<typename T>
  using Vector4 = VectorN<4, T>;
  template<typename T>
  using Matrix = glm::mat<4, 4, T, glm::defaultp>;
  template<typename T>
  using Matrix3 = glm::mat<3, 3, T, glm::defaultp>;
  template<typename T>
  using Quaternion = glm::qua<T, glm::defaultp>;
  
  using Vec2 = Vector2<float>;
  using Vec3 = Vector3<float>;
  using Vec4 = Vector4<float>;

  using Vec2d = Vector2<double>;
  using Vec3d = Vector3<double>;
  using Vec4d = Vector4<double>;

  using Vec2i = Vector2<int32_t>;
  using Vec3i = Vector3<int32_t>;
  using Vec4i = Vector4<int32_t>;

  using Vec2u = Vector2<uint32_t>;
  using Vec3u = Vector3<uint32_t>;
  using Vec4u = Vector4<uint32_t>;

  using Vec2i64 = Vector2<int64_t>;
  using Vec3i64 = Vector3<int64_t>;
  using Vec4i64 = Vector4<int64_t>;
  
  using Vec2u64 = Vector2<uint64_t>;
  using Vec3u64 = Vector3<uint64_t>;
  using Vec4u64 = Vector4<uint64_t>;

  using Quat  = Quaternion<float>;
  using Quatd = Quaternion<double>;

  using Mat4  = Matrix<float>;
  using Mat4d = Matrix<double>;
  
  using Mat3  = Matrix3<float>;
  using Mat3d = Matrix3<double>;

  // clang-format off
  template<typename T>
  struct Reflect<Vector2<T>> {
    static inline constexpr auto get() {
      return makeReflection<Vector2<T>>(
        BFC_REFLECT(Vector2<T>, x),
        BFC_REFLECT(Vector2<T>, y)
      );
    }
  };
  
  template<typename T>
  struct Reflect<Vector3<T>> {
    static inline constexpr auto get() {
      return makeReflection<Vector3<T>>(
        BFC_REFLECT(Vector3<T>, x),
        BFC_REFLECT(Vector3<T>, y),
        BFC_REFLECT(Vector3<T>, z)
      );
    }
  };
  
  template<typename T>
  struct Reflect<Vector4<T>> {
    static inline constexpr auto get() {
      return makeReflection<Vector4<T>>(
        BFC_REFLECT(Vector4<T>, x),
        BFC_REFLECT(Vector4<T>, y),
        BFC_REFLECT(Vector4<T>, z),
        BFC_REFLECT(Vector4<T>, w)
      );
    }
  };
  
  template<typename T>
  struct Reflect<Quaternion<T>> {
    static inline constexpr auto get() {
      return makeReflection<Quaternion<T>>(
        BFC_REFLECT(Quaternion<T>, x),
        BFC_REFLECT(Quaternion<T>, y),
        BFC_REFLECT(Quaternion<T>, z),
        BFC_REFLECT(Quaternion<T>, w)
      );
    }
  };

  // clang-format on

  template<typename T>
  inline constexpr uint64_t hash(Vector2<T> const& o) {
    return hash(o.x, o.y);
  }

  template<typename T>
  inline constexpr uint64_t hash(Vector3<T> const & o) {
    return hash(o.x, o.y, o.z);
  }

  template<typename T>
  inline constexpr uint64_t hash(Vector4<T> const & o) {
    return hash(o.x, o.y, o.z, o.w);
  }

  template<typename T>
  inline constexpr uint64_t hash(Quaternion<T> const & o) {
    return hash(o.x, o.y, o.z, o.w);
  }

  template<typename T>
  inline constexpr uint64_t hash(Matrix<T> const & o) {
    return hash(o[0], o[1], o[2], o[3]);
  }

  namespace math {
    template<class T>
    inline constexpr Vector3<T> up = Vector3<T>(0, 1, 0);
    template<class T>
    inline constexpr Vector3<T> right = Vector3<T>(1, 0, 0);
    template<class T>
    inline constexpr Vector3<T> forward = Vector3<T>(0, 0, -1);

    inline double radians(double value) {
      return glm::radians(value);
    }

    inline float radians(float value) {
      return glm::radians(value);
    }

    inline double degrees(double value) {
      return glm::degrees(value);
    }

    inline float degrees(float value) {
      return glm::degrees(value);
    }

    template<typename T>
    T random(T min, T max) {
      return min + rand() / (RAND_MAX * (max - min));
    }

    template<typename T>
    T degrees(T const & iterable) {
      T ret = iterable;
      for (auto & val : (T&)ret) {
        val = glm::degrees(val);
      }
      return ret;
    }

    template<typename T>
    T radians(T const & iterable) {
      T ret = iterable;
      for (auto & val : (T&)ret) {
        val = glm::radians(val);
      }
      return ret;
    }

    template<typename T>
    Vector2<T> min(Vector2<T> const & a, Vector2<T> const & b) {
      return {min(a.x, b.x), min(a.y, b.y)};
    }

    template<typename T>
    Vector3<T> min(Vector3<T> const & a, Vector3<T> const & b) {
      return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
    }

    template<typename T>
    Vector4<T> min(Vector4<T> const & a, Vector4<T> const & b) {
      return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)};
    }

    template<typename T>
    Vector2<T> max(Vector2<T> const & a, Vector2<T> const & b) {
      return {max(a.x, b.x), max(a.y, b.y)};
    }

    template<typename T>
    Vector3<T> max(Vector3<T> const & a, Vector3<T> const & b) {
      return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
    }

    template<typename T>
    Vector4<T> max(Vector4<T> const & a, Vector4<T> const & b) {
      return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)};
    }

    template<typename T>
    Vector2<T> abs(Vector2<T> const & o) {
      return {glm::abs(o.x), glm::abs(o.y)};
    }

    template<typename T>
    Vector3<T> abs(Vector3<T> const & o) {
      return {glm::abs(o.x), glm::abs(o.y), glm::abs(o.z) };
    }

    template<typename T>
    Vector4<T> abs(Vector4<T> const & o) {
      return {glm::abs(o.x), glm::abs(o.y), glm::abs(o.z), glm::abs(o.w)};
    }

    template<typename T>
    T length(Vector2<T> const & o) {
      return glm::sqrt(glm::dot(o, o));
    }

    template<typename T>
    T length(Vector3<T> const & o) {
      return glm::sqrt(glm::dot(o, o));
    }

    template<typename T>
    T length(Vector4<T> const & o) {
      return glm::sqrt(glm::dot(o, o));
    }

    template<typename T>
    T minComponent(Vector2<T> const & o) {
      return min(o.x, o.y);
    }

    template<typename T>
    T minComponent(Vector3<T> const & o) {
      return min(o.x, o.y, o.z);
    }

    template<typename T>
    T minComponent(Vector4<T> const & o) {
      return min(o.x, o.y, o.z, o.w);
    }

    template<typename T>
    T maxComponent(Vector2<T> const & o) {
      return max(o.x, o.y);
    }

    template<typename T>
    T maxComponent(Vector3<T> const & o) {
      return max(o.x, o.y, o.z);
    }

    template<typename T>
    T maxComponent(Vector4<T> const & o) {
      return max(o.x, o.y, o.z, o.w);
    }

    template<typename T>
    bool isFinite(T const& value) {
      return glm::isfinite(value);
    }

    template<typename T>
    bool isFinite(Vector2<T> const & value) {
      return glm::isfinite(value.x) && glm::isfinite(value.y);
    }

    template<typename T>
    bool isFinite(Vector3<T> const & value) {
      return glm::isfinite(value.x) && glm::isfinite(value.y) && glm::isfinite(value.z);
    }

    template<typename T>
    bool isFinite(Vector4<T> const & value) {
      return glm::isfinite(value.x) && glm::isfinite(value.y) && glm::isfinite(value.z) & glm::isfinite(value.w);
    }

    template<int64_t I, typename T>
    T pow(T const& base) {
      if constexpr (I < 0) {
        static_assert(false, "I cannot be less than 0");
      } else if constexpr (I == 0) {
        return T(1);
      } else {
        return base * pow<I - 1>(base);
      }
    }

    template<typename T>
    Matrix<T> translation(T x, T y, T z) {
      return glm::translation({x, y, z});
    }

    template<typename T>
    Matrix<T> translation(Vector3<T> const & pos) {
      return glm::translate(pos);
    }

    template<typename T>
    Matrix<T> rotationYPR(Vector3<T> const & ypr) {
      return glm::yawPitchRoll(ypr.x, ypr.y, ypr.z);
    }

    template<typename T>
    Matrix<T> rotationYPR(T y, T p, T r) {
      return glm::yaw(y, p, t);
    }

    template<typename T>
    Matrix<T> scale(Vector3<T> const & amount) {
      return glm::scale(amount);
    }

    template<typename T>
    Matrix<T> scale(T x, T y, T z) {
      return scale({x, y, z});
    }

    template<typename T>
    Matrix<T> scale(T const & amount) {
      return glm::scale(Vector3<T>(amount));
    }

    template<typename T>
    T identity() {
      return glm::identity<T>();
    }

    template<typename T>
    void calculateAxes(Vector3<T> const & forward, Vector3<T> * pUp, Vector3<T> * pRight) {
      Vec3 up, right;
      if (glm::abs(glm::dot(forward, math::up<float>)) - 1 > 0.0001) {
        right = glm::cross(forward, math::up<float>);
        up    = glm::cross(right, forward);
      } else {
        up    = glm::cross(math::right<float>, forward);
        right = glm::cross(forward, up);
      }

      *pUp    = glm::normalize(up);
      *pRight = glm::normalize(right);
    }

    template<typename T>
    Vector2<T> solveQuadratic(T a, T b, T c) {
      T a_2  = a * 2;
      T root = (T)sqrt(b * b - 4 * a * c)/a_2;
      T b_2a = b / a_2;
      return {b_2a + root, b_2a - root};
    }
  } // namespace math
} // namespace bfc
