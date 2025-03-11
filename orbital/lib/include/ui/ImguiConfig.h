#pragma once

#include "math/MathTypes.h"
#include "render/GraphicsDevice.h"

#define IMGUI_API BFC_API
#define IMGUI_DEFINE_MATH_OPERATORS

#define IM_VEC2_CLASS_EXTRA                                                                                                                                    \
  constexpr ImVec2(const bfc::Vec2 & f)                                                                                                                        \
    : x(f.x)                                                                                                                                                   \
    , y(f.y) {}                                                                                                                                                \
  constexpr ImVec2(const bfc::Vec2i & f)                                                                                                                       \
    : x((float)f.x)                                                                                                                                            \
    , y((float)f.y) {}                                                                                                                                         \
  constexpr ImVec2(const bfc::Vec2d & f)                                                                                                                       \
    : x((float)f.x)                                                                                                                                            \
    , y((float)f.y) {}                                                                                                                                         \
  constexpr ImVec2(const bfc::Vec2i64 & f)                                                                                                                     \
    : x((float)f.x)                                                                                                                                            \
    , y((float)f.y) {}                                                                                                                                         \
  operator bfc::Vec2() const {                                                                                                                                 \
    return bfc::Vec2(x, y);                                                                                                                                    \
  }                                                                                                                                                            \
  operator bfc::Vec2d() const {                                                                                                                                \
    return bfc::Vec2d(x, y);                                                                                                                                   \
  }                                                                                                                                                            \
  operator bfc::Vec2i() const {                                                                                                                                \
    return bfc::Vec2i(x, y);                                                                                                                                   \
  }                                                                                                                                                            \
  operator bfc::Vec2i64() const {                                                                                                                              \
    return bfc::Vec2i64(x, y);                                                                                                                                 \
  }

#define IM_VEC4_CLASS_EXTRA                                                                                                                                    \
  constexpr ImVec4(const bfc::Vec4 & f)                                                                                                                        \
    : x(f.x)                                                                                                                                                   \
    , y(f.y)                                                                                                                                                   \
    , z(f.z)                                                                                                                                                   \
    , w(f.w) {}                                                                                                                                                \
  constexpr ImVec4(const bfc::Vec4i & f)                                                                                                                       \
    : x((float)f.x)                                                                                                                                            \
    , y((float)f.y)                                                                                                                                            \
    , z((float)f.z)                                                                                                                                            \
    , w((float)f.w) {}                                                                                                                                         \
  constexpr ImVec4(const bfc::Vec4d & f)                                                                                                                       \
    : x((float)f.x)                                                                                                                                            \
    , y((float)f.y)                                                                                                                                            \
    , z((float)f.z)                                                                                                                                            \
    , w((float)f.w) {}                                                                                                                                         \
  constexpr ImVec4(const bfc::Vec4i64 & f)                                                                                                                     \
    : x((float)f.x)                                                                                                                                            \
    , y((float)f.y)                                                                                                                                            \
    , z((float)f.z)                                                                                                                                            \
    , w((float)f.w) {}                                                                                                                                         \
  operator bfc::Vec4() const {                                                                                                                                 \
    return bfc::Vec4(x, y, z, w);                                                                                                                              \
  }                                                                                                                                                            \
  operator bfc::Vec4i() const {                                                                                                                                \
    return bfc::Vec4i(x, y, z, w);                                                                                                                             \
  }                                                                                                                                                            \
  operator bfc::Vec4d() const {                                                                                                                                \
    return bfc::Vec4d(x, y, z, w);                                                                                                                             \
  }                                                                                                                                                            \
  operator bfc::Vec4i64() const {                                                                                                                              \
    return bfc::Vec4i64(x, y, z, w);                                                                                                                           \
  }

namespace bfc {
  namespace external {
    struct ImGuiTextureRef {
      ImGuiTextureRef() = default;
      ImGuiTextureRef(ImGuiTextureRef const & o);
      ImGuiTextureRef & operator=(ImGuiTextureRef const & o);
      ImGuiTextureRef(::bfc::graphics::TextureRef const & pTexture);
      operator uint64_t() const;

      uint64_t index = 0;
    };
  }
}
// Override texture type to be a bfc::GraphicsResource
#define ImTextureID ::bfc::external::ImGuiTextureRef
