#include "media/Pixel.h"

namespace bfc {
  int64_t getPixelFormatStride(PixelFormat const & format) {
    switch (format) {
    case PixelFormat_RGBAu8: return 4;
    case PixelFormat_RGBu8: return 3;
    case PixelFormat_Ru8: return 1;
    case PixelFormat_Lu8: return 1;
    case PixelFormat_LAu8: return 2;
    case PixelFormat_RGBAu16: return 4 * 2;
    case PixelFormat_RGBAf16: return 4 * 2;
    case PixelFormat_RGBAf32: return 4 * 4;
    case PixelFormat_Rf32: return 4;
    case PixelFormat_RGBAf64: return 4 * 8;
    }
    return 0;
  }
} // namespace bfc
