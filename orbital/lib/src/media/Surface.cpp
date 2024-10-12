#include "media/Surface.h"
#include "core/Stream.h"
#include "core/File.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FREE(p)    ::bfc::mem::free(p)
#define STBI_MALLOC(sz) ::bfc::mem::alloc(sz)
#define STBI_REALLOC(p, newsz) ::bfc::mem::realloc(p, newsz)
#include "../../../vendor/stb/stb_image.h"

namespace bfc {
  namespace media {
    void convertSurface(Surface* pDst, Surface const& src) {
      switch (src.format) {
      case PixelFormat_Ru8:
        convertSurface<Ru8>(pDst, src.pBuffer, src.size, src.pitch);
        break;
      case PixelFormat_Lu8:
        convertSurface<Lu8>(pDst, src.pBuffer, src.size, src.pitch);
        break;
      case PixelFormat_LAu8:
        convertSurface<LAu8>(pDst, src.pBuffer, src.size, src.pitch);
        break;
      case PixelFormat_RGBu8:
        convertSurface<RGBu8>(pDst, src.pBuffer, src.size, src.pitch);
        break;
      case PixelFormat_RGBAu8:
        convertSurface<RGBAu8>(pDst, src.pBuffer, src.size, src.pitch);
        break;
      case PixelFormat_Rf32:
        convertSurface<Rf32>(pDst, src.pBuffer, src.size, src.pitch);
        break;
      case PixelFormat_RGBAf32:
        convertSurface<RGBAf32>(pDst, src.pBuffer, src.size, src.pitch);
        break;
      }
    }

    void* allocateSurface(Surface const& surface) {
      int64_t rowLength  = surface.pitch == 0 ? surface.size.x * getPixelFormatStride(surface.format) : surface.pitch;
      int64_t bufferSize = rowLength * surface.size.y * surface.size.z;
      return bfc::mem::alloc(bufferSize);
    }

    int64_t calculateSurfaceSize(Surface const & surface) {
      int64_t rowLength  = surface.pitch == 0 ? surface.size.x * getPixelFormatStride(surface.format) : surface.pitch;
      return rowLength * surface.size.y * surface.size.z;
    }

    bool canLoadSurface(StringView const & extension) {
      static const Vector<String> supported = { "jpeg", "jpg", "png", "bmp", "hdr", "psd", "tga", "gif", "pic", "pgm", "ppm" };

      for (auto const& o : supported) {
        if (o.equals(extension, true)) {
          return true;
        }
      }

      return false;
    }

    bool loadSurface(URI const& path, Surface* pSurface) {
      Ref<Stream> pStream = openURI(path, FileMode_ReadBinary);
      if (pStream == nullptr) {
        return false;
      }
      return loadSurface(pStream.get(), pSurface);
    }

    bool loadSurface(Stream* pStream, Surface* pSurface) {
      // ASSERT
      BFC_ASSERT(pStream->readable(), "Stream is not readable.");
      BFC_ASSERT(pStream->seekable(), "Stream is not seekable.");

      stbi_io_callbacks reader;
      reader.read = [](void* pUser, char* data, int size) { return (int)((Stream*)pUser)->read(data, size); };
      reader.eof = [](void* pUser) { return (int)((Stream*)pUser)->eof(); };
      reader.skip = [](void *pUser, int skip) { ((Stream *)pUser)->seek(skip); };

      int numComponents = 0;
      uint8_t* pData = stbi_load_from_callbacks(&reader, pStream, &pSurface->size.x, &pSurface->size.y, &numComponents, 0);

      if (pData == nullptr)
        return false;

      pSurface->pBuffer = pData;
      pSurface->pitch = 0;
      
      switch (numComponents)
      {
      case 1: pSurface->format = PixelFormat_Lu8; break;
      case 2: pSurface->format = PixelFormat_LAu8; break;
      case 3: pSurface->format = PixelFormat_RGBu8; break;
      case 4: pSurface->format = PixelFormat_RGBAu8; break;
      }
      return true;
    }

    void * getSurfacePixel(Surface const & surface, Vec3i const & coord) {
      if (surface.pBuffer == nullptr) {
        return nullptr;
      }

      int64_t pitch = getSurfacePitch(surface);
      int64_t index = coord.x * getPixelFormatStride(surface.format) + coord.y * pitch + coord.z * surface.size.y * pitch;
      return (uint8_t*)surface.pBuffer + index;
    }

    int64_t getSurfacePitch(Surface const & surface) {
      return surface.pitch == 0 ? surface.size.x * getPixelFormatStride(surface.format) : surface.pitch;
    }

    void Surface::free()
    {
      mem::free(pBuffer);
      pBuffer = 0;
    }
    Surface Surface::slice(int64_t z) const {
      Surface ret;
      ret.size    = {size.x, size.y, 1 };
      ret.pitch   = pitch;
      ret.format  = format;
      ret.pBuffer = pBuffer;
      if (ret.pBuffer != nullptr) {
        ret.pBuffer = (uint8_t *)ret.pBuffer + getSurfacePitch(*this) * size.y * z;
      }
      return ret;
    }
  } // namespace media

  int64_t write(Stream * pStream, media::Surface const * pValue, int64_t count) {
    void *      pBuffer = nullptr;
    PixelFormat format  = PixelFormat_Unknown;
    Vec3i       size    = {0, 0, 1};
    int64_t     pitch   = 0;
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].format) && pStream->write(pValue[i].size) && pStream->write(pValue[i].pitch))) {
        return i;
      }

      int64_t bufLen = calculateSurfaceSize(pValue[i]);
      if (pStream->write(pValue[i].pBuffer, bufLen) != bufLen) {
        return i;
      }
    }
    return count;
  }

  int64_t read(Stream * pStream, media::Surface * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].format) && pStream->read(&pValue[i].size) && pStream->read(&pValue[i].pitch))) {
        return i;
      }
      pValue[i].pBuffer = allocateSurface(pValue[i]);
      int64_t bufLen    = calculateSurfaceSize(pValue[i]);
      if (pStream->read(pValue[i].pBuffer, bufLen) != bufLen) {
        return i;
      }
    }

    return count;
  }
}
