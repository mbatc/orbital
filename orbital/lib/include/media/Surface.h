#pragma once

#include "Pixel.h"

#include "../core/Filename.h"
#include "../core/Memory.h"
#include "../core/URI.h"
#include "../math/MathTypes.h"

namespace bfc {
  class Stream;

  namespace media {
    class BFC_API Surface {
    public:
      void *      pBuffer = nullptr;
      PixelFormat format  = PixelFormat_Unknown;
      Vec3i       size    = {0, 0, 1};
      int64_t     pitch   = 0;

      void free();

      Surface slice(int64_t z) const;
    };

    /// Allocate a buffer for `surface`.
    /// The buffer is large enough to contain the image format and size described by `surface`.
    BFC_API void * allocateSurface(Surface const & surface);

    /// Calculate the size of a surface in bytes.
    BFC_API int64_t calculateSurfaceSize(Surface const & surface);

    /// Test if a surface can be loaded from a file with the specified extension.
    BFC_API bool canLoadSurface(StringView const & extension);

    BFC_API bool loadSurface(URI const & path, Surface * pSurface);

    BFC_API bool loadSurface(Stream * pStream, Surface * pSurface);

    BFC_API void * getSurfacePixel(Surface const & surface, Vec3i const & coord);

    BFC_API int64_t getSurfacePitch(Surface const & surface);

    inline int64_t calculatePixelOffset(Vec3i pos, Vec3i size, int64_t stride, int64_t pitch) {
      return pos.x * stride + (pos.y + pos.z * size.y) * pitch;
    }

    template<typename DstFormat, typename SrcFormat = DstFormat>
    void convertSurface(void ** ppDst, void const * pSrc, Vec3i dstSize, Vec3i srcSize, int64_t dstPitch = 0, int64_t srcPitch = 0) {
      using DstT = Colour<DstFormat>;
      using SrcT = Colour<SrcFormat>;

      // Ensure pitch is correct
      if (dstPitch == 0)
        dstPitch = dstSize.x * sizeof(DstT);
      if (srcPitch == 0)
        srcPitch = srcSize.x * sizeof(SrcT);

      // Allocate a destination buffer if none is provided
      if (*ppDst == nullptr)
        *ppDst = mem::alloc(dstPitch * dstSize.y * std::max(dstSize.z, 1));

      if (dstSize != srcSize) {
        // Different size. Re-sample surface regardless of format.
        DstT *       pDstRow  = (DstT *)*ppDst;
        SrcT const * pSrcPtr  = (SrcT const *)pSrc;
        Vec3i        coord    = { 0, 0, 0 };
        Vec3         srcRatio = Vec3(srcSize) / Vec3(dstSize);
        for (coord.z = 0; coord.z < dstSize.z; ++coord.z) {
          for (coord.y = 0; coord.y < dstSize.y; ++coord.y) {
            for (coord.x = 0; coord.x < dstSize.x; ++coord.x) {
              int64_t srcOffset = calculatePixelOffset(Vec3i(Vec3(coord) * srcRatio), srcSize, sizeof(SrcT), srcPitch);

              pDstRow[coord.x] = *(SrcT const *)(((uint8_t const *)pSrcPtr) + srcOffset);
            }

            pDstRow = (DstT *)(((uint8_t *)pDstRow) + dstPitch);
          }
        }
      } else {
        // Same size.
        if constexpr (std::is_same_v<DstT, SrcT>) {
          if (dstPitch == srcPitch && dstSize == srcSize) {
            // Same format, same pitch. Copy entire buffer.
            memcpy(*ppDst, pSrc, dstPitch * srcSize.y * std::max(srcSize.z, 1));
          } else { // Copy row by row
            // Same format, different pitch. Copy row by row.
            DstT *       pDstRow = (DstT *)*ppDst;
            SrcT const * pSrcRow = (SrcT const *)pSrc;
            for (int64_t z = 0; z < dstSize.z; ++z) {
              for (int64_t y = 0; y < dstSize.y; ++y) {
                for (int64_t x = 0; x < dstSize.x; ++x)
                  memcpy(pDstRow, pSrcRow, srcPitch);
                pDstRow = (DstT *)(((uint8_t *)pDstRow) + dstPitch);
                pSrcRow = (SrcT *)(((uint8_t *)pSrcRow) + srcPitch);
              }
            }
          }
        } else {
          // Different format. Re-sample surface.
          DstT *       pDstRow = (DstT *)*ppDst;
          SrcT const * pSrcRow = (SrcT const *)pSrc;
          for (int64_t z = 0; z < dstSize.z; ++z) {
            for (int64_t y = 0; y < dstSize.y; ++y) {
              for (int64_t x = 0; x < dstSize.x; ++x) {
                pDstRow[x] = pSrcRow[x];
              }

              pDstRow = (DstT *)(((uint8_t *)pDstRow) + dstPitch);
              pSrcRow = (SrcT *)(((uint8_t *)pSrcRow) + srcPitch);
            }
          }
        }
      }
    }

    template<typename SrcFormat>
    void convertSurface(Surface * pDst, void const * pSrc, Vec3i size, int64_t srcPitch = 0) {
      switch (pDst->format) {
      case PixelFormat_Lu8: convertSurface<Lu8, SrcFormat>(&pDst->pBuffer, pSrc, pDst->size, size, pDst->pitch, srcPitch); break;
      case PixelFormat_LAu8: convertSurface<LAu8, SrcFormat>(&pDst->pBuffer, pSrc, pDst->size, size, pDst->pitch, srcPitch); break;
      case PixelFormat_Ru8: convertSurface<Ru8, SrcFormat>(&pDst->pBuffer, pSrc, pDst->size, size, pDst->pitch, srcPitch); break;
      case PixelFormat_RGBu8: convertSurface<RGBu8, SrcFormat>(&pDst->pBuffer, pSrc, pDst->size, size, pDst->pitch, srcPitch); break;
      case PixelFormat_RGBAu8: convertSurface<RGBAu8, SrcFormat>(&pDst->pBuffer, pSrc, pDst->size, size, pDst->pitch, srcPitch); break;
      case PixelFormat_Rf32: convertSurface<Rf32, SrcFormat>(&pDst->pBuffer, pSrc, pDst->size, size, pDst->pitch, srcPitch); break;
      case PixelFormat_RGBAf32: convertSurface<RGBAf32, SrcFormat>(&pDst->pBuffer, pSrc, pDst->size, size, pDst->pitch, srcPitch); break;
      }
    }

    BFC_API void convertSurface(Surface * pDst, Surface const & src);
  } // namespace media

  BFC_API int64_t write(Stream * pStream, media::Surface const * pValue, int64_t count);
  BFC_API int64_t read(Stream * pStream, media::Surface * pValue, int64_t count);
} // namespace bfc
