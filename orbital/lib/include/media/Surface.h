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
    };

    BFC_API void * allocateSurface(Surface const & surface);

    BFC_API int64_t calculateSurfaceSize(Surface const & surface);

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

      if constexpr (std::is_same_v<DstT, SrcT>) {
        if (dstPitch == srcPitch && dstSize == srcSize) {
          // Same format, same size, same pitch. Copy entire buffer.
          memcpy(*ppDst, pSrc, dstPitch * srcSize.y * std::max(srcSize.z, 1));
        } else { // Copy row by row
          if (dstSize == srcSize) {
            // Same format, same size, different pitch. Copy row by row.
            DstT *       pDstRow = (DstT *)*ppDst;
            SrcT const * pSrcRow = (SrcT const *)pSrc;
            for (int64_t z = 0; z < dstSize.z; ++z) {
              for (int64_t y = 0; y < dstSize.y; ++y) {
                for (int64_t x = 0; x < dstSize.x; ++x)
                  memcpy(pDstRow, pSrcRow, dstPitch);
                pDstRow = (DstT *)(((uint8_t *)pDstRow) + dstPitch);
                pSrcRow = (SrcT *)(((uint8_t *)pSrcRow) + srcPitch);
              }
            }
          } else {
            // Same format, different size. Re-sample surface.
            DstT *       pDstPtr = (DstT *)*ppDst;
            SrcT const * pSrcPtr = (SrcT const *)pSrc;
            Vec3d        step    = Vec3d(1.0) / Vec3d(dstSize);
            for (double z = 0; z < 1; z += step.z) {
              for (double y = 0; y < 1; y += step.y) {
                for (double x = 0; x < 1; x += step.x) {
                  int64_t dstOffset = calculatePixelOffset(Vec3i(Vec3d{x, y, z} * Vec3d(dstSize)), dstSize, sizeof(DstT), dstPitch);
                  int64_t srcOffset = calculatePixelOffset(Vec3i(Vec3d{x, y, z} * Vec3d(srcSize)), srcSize, sizeof(SrcT), srcPitch);

                  *(DstT *)(((uint8_t *)pDstPtr) + dstOffset) = *(SrcT *)(((uint8_t *)pSrcPtr) + srcOffset);
                }
              }
            }
          }
        }
      } else {
        if (dstSize == srcSize) {
          // Different format, same size. Re-sample surface.
          DstT *       pDstRow = (DstT *)*ppDst;
          SrcT const * pSrcRow = (SrcT const *)pSrc;
          for (int64_t z = 0; z < dstSize.z; ++z) {
            for (int64_t y = 0; y < dstSize.y; ++y) {
              for (int64_t x = 0; x < dstSize.x; ++x) {
                if constexpr (DstT::hasR)
                  pSrcRow[x].getR(pDstRow[x].r);
                if constexpr (DstT::hasG)
                  pSrcRow[x].getG(pDstRow[x].g);
                if constexpr (DstT::hasB)
                  pSrcRow[x].getB(pDstRow[x].b);
                if constexpr (DstT::hasA)
                  pSrcRow[x].getA(pDstRow[x].a);
              }

              pDstRow = (DstT *)(((uint8_t *)pDstRow) + dstPitch);
              pSrcRow = (SrcT *)(((uint8_t *)pSrcRow) + srcPitch);
            }
          }
        } else {
          // Different format, different size. Re-sample surface.
            // Same format, different size. Re-sample surface.
          DstT *       pDstPtr = (DstT *)*ppDst;
          SrcT const * pSrcPtr = (SrcT const *)pSrc;
          Vec3d        step    = Vec3d(1.0) / Vec3d(dstSize);
          for (double z = 0; z < 1; z += step.z) {
            for (double y = 0; y < 1; y += step.y) {
              for (double x = 0; x < 1; x += step.x) {
                int64_t dstOffset = calculatePixelOffset(Vec3i(Vec3d{x, y, z} * Vec3d(dstSize)), dstSize, sizeof(DstT), dstPitch);
                int64_t srcOffset = calculatePixelOffset(Vec3i(Vec3d{x, y, z} * Vec3d(srcSize)), srcSize, sizeof(SrcT), srcPitch);

                *(DstT *)(((uint8_t *)pDstPtr) + dstOffset) = *(SrcT *)(((uint8_t *)pSrcPtr) + srcOffset);
              }
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
