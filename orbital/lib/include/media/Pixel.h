#pragma once

#include "../core/Core.h"
#include "../math/MathTypes.h"

#include <type_traits>
#include <limits>

BFC_DEFINE_MEMBER_CHECK(r);
BFC_DEFINE_MEMBER_CHECK(g);
BFC_DEFINE_MEMBER_CHECK(b);
BFC_DEFINE_MEMBER_CHECK(a);

namespace bfc {
  enum PixelFormat {
    PixelFormat_Unknown = -1,

    // UInt8 formats
    PixelFormat_RGBAu8,
    PixelFormat_RGBu8,
    PixelFormat_Ru8,
    PixelFormat_Lu8,
    PixelFormat_LAu8,

    // UInt16 formats
    PixelFormat_RGBAu16,

    // 16-bit floating point formats
    PixelFormat_RGBAf16,

    // 32-bit Floating point formats
    PixelFormat_RGBAf32,
    PixelFormat_RGBf32,
    PixelFormat_Rf32,

    // 64-bit floating point formats
    PixelFormat_RGBAf64,

    PixelFormat_Count,
  };

  BFC_API int64_t getPixelFormatStride(PixelFormat const& format);

  struct RGBAu8 {
    static constexpr PixelFormat FormatID = PixelFormat_RGBAu8;

    RGBAu8() = default;

    RGBAu8(uint32_t packed)
      : r(0)
      , g(0)
      , b(0)
      , a(0)
    {
      *(uint32_t *)this = packed;
    }

    constexpr RGBAu8(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {}

    uint8_t r, g, b, a;
  };

  struct RGBAu16 {
    static constexpr PixelFormat FormatID = PixelFormat_RGBAu16;

    RGBAu16() = default;

    uint16_t r, g, b, a;
  };

  struct RGBu8 {
    static constexpr PixelFormat FormatID = PixelFormat_RGBu8;

    uint8_t r, g, b;
  };

  struct Ru8 {
    static constexpr PixelFormat FormatID = PixelFormat_Ru8;

    uint8_t r;
  };

  struct Lu8 {
    static constexpr PixelFormat FormatID = PixelFormat_Lu8;
    Lu8() = default;

    constexpr Lu8(uint8_t luminance) : grayscale(luminance) {}

    union {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint8_t grayscale;
    };
  };

  struct LAu8 {
    static constexpr PixelFormat FormatID = PixelFormat_LAu8;

    LAu8() = default;

    constexpr LAu8(uint8_t luminance, uint8_t alpha) : grayscale(luminance) , alpha(alpha) {}

    union {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      uint8_t grayscale;
    };
    uint8_t alpha;
  };

  struct RGBAf32 {
    static constexpr PixelFormat FormatID = PixelFormat_RGBAf32;

    RGBAf32() = default;

    constexpr RGBAf32(Vec4 vec)
        : r(vec.x), g(vec.y), b(vec.z), a(vec.w) {
    }

    float r, g, b, a;
  };

  struct RGBf32 {
    static constexpr PixelFormat FormatID = PixelFormat_RGBf32;

    RGBf32() = default;

    constexpr RGBf32(Vec4 vec)
      : r(vec.x)
      , g(vec.y)
      , b(vec.z) {}

    float r, g, b;
  };

  struct Rf32 {
    static constexpr PixelFormat FormatID = PixelFormat_Rf32;

    float r;
  };

  struct RGBAf64 {
    static constexpr PixelFormat FormatID = PixelFormat_RGBAf64;

    RGBAf64() = default;

    constexpr RGBAf64(Vec4d vec)
      : r(vec.x)
      , g(vec.y)
      , b(vec.z)
      , a(vec.w) {}

    double r, g, b, a;
  };

  template <typename T>
  struct pixel_default {
    static constexpr T R = 0;
    static constexpr T G = 0;
    static constexpr T B = 0;
    static constexpr T A = std::numeric_limits<T>::max();
  };

  template <>
  struct pixel_default<float> {
    static constexpr float R = 0;
    static constexpr float G = 0;
    static constexpr float B = 0;
    static constexpr float A = 1.0f;
  };

  template<>
  struct pixel_default<double> {
    static constexpr double R = 0;
    static constexpr double G = 0;
    static constexpr double B = 0;
    static constexpr double A = 1.0f;
  };

  template <typename T>
  struct pixel_range {
    static constexpr T Min = 0;
    static constexpr T Max = std::numeric_limits<T>::max();
  };

  template <>
  struct pixel_range<float> {
    static constexpr float Min = 0;
    static constexpr float Max = 1.0f;
  };

  template<>
  struct pixel_range<double> {
    static constexpr float Min = 0;
    static constexpr float Max = 1.0f;
  };

  template <typename U, typename T>
  constexpr U convertPixelChannel(T const& src) {
    if constexpr (std::is_same_v<T, U>)
      return src;
    else
      return pixel_range<U>::Min + (U)((src - pixel_range<T>::Min) * double((pixel_range<U>::Max - pixel_range<U>::Min) / (pixel_range<T>::Max - pixel_range<T>::Min)));
  }

  template <typename Format>
  struct Colour : public Format {
    using Format::Format;

    static constexpr bool hasR = BFC_HAS_MEMBER(Format, r);
    static constexpr bool hasG = BFC_HAS_MEMBER(Format, g);
    static constexpr bool hasB = BFC_HAS_MEMBER(Format, b);
    static constexpr bool hasA = BFC_HAS_MEMBER(Format, a);

    template <typename T>
    void getR(T& dst) const {
      if constexpr (hasR)
        dst = convertPixelChannel<T>(r);
      else
        dst = pixel_default<T>::R;
    }

    template <typename T>
    void getG(T& dst) const {
      if constexpr (hasG)
        dst = convertPixelChannel<T>(g);
      else
        dst = pixel_default<T>::G;
    }

    template <typename T>
    void getB(T& dst) const {
      if constexpr (hasB)
        dst = convertPixelChannel<T>(b);
      else
        dst = pixel_default<T>::B;
    }

    template <typename T>
    void getA(T& dst) const {
      if constexpr (hasA)
        dst = convertPixelChannel<T>(a);
      else
        dst = pixel_default<T>::A;
    }

    template <typename T>
    void setR(T const& src) {
      if constexpr (hasR)
        r = convertPixelChannel<decltype(r)>(src);
      else
        BFC_UNUSED(src);
    }

    template <typename T>
    void setG(T const& src) {
      if constexpr (hasG)
        g = convertPixelChannel<decltype(g)>(src);
      else
        BFC_UNUSED(src);
    }

    template <typename T>
    void setB(T const& src) {
      if constexpr (hasB)
        b = convertPixelChannel<decltype(b)>(src);
      else
        BFC_UNUSED(src);
    }

    template <typename T>
    void setA(T const& src) {
      if constexpr (hasA)
        a = convertPixelChannel<decltype(a)>(src);
      else
        BFC_UNUSED(src);
    }

    template<typename DstFormat>
    operator Colour<DstFormat>() const {
      Colour<DstFormat> pix;
      if constexpr (pix.hasR) getR(pix.r);
      if constexpr (pix.hasG) getG(pix.g);
      if constexpr (pix.hasB) getB(pix.b);
      if constexpr (pix.hasA) getA(pix.a);
      return pix;
    }
  };
}
