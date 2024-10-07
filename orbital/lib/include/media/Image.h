#pragma once

#include "../Core/Vector.h"
#include "Surface.h"

namespace bfc {
  namespace media {
    template <typename Format>
    class Image {
    public:
      static constexpr PixelFormat FormatID = Format::FormatID;

      using PixelT = Colour<Format>;
      
      Image() = default;

      Image(Vec2i size, PixelT initialValue = { 0 }) {
        m_pixels.resize(size.x * size.y, initialValue);
        m_size = size;
      }

      Image(PixelT* pPixels, Vec2i size, bool takeData = false) {
        load(pPixels, size, takeData);
      }

      Image(Filename const &file) {
        load(file);
      }

      Image(Surface const& surface, int64_t slice = 0) {
        load(surface, slice);
      }

      bool load(Filename const& file) {
        Surface surface;
        if (!loadSurface(file, &surface))
          return false;
        load(surface, 0, true);
        return true;
      }

      void load(Surface surface, int64_t slice = 0, bool takeData = false) {
        Surface dstSurface;
        dstSurface.format = FormatID;
        dstSurface.pitch = 0;
        dstSurface.size   = surface.size;

        if (surface.format != FormatID) { // Convert to correct format
          convertSurface(&dstSurface, surface);
          if (takeData) 
            surface.free(); // Caller isn't managing 'surface' memory, free it
          takeData = true;
        }
        else {
          dstSurface = surface;
        }

        // Use memory allocated during conversion
        load((PixelT*)dstSurface.pBuffer, dstSurface.size, takeData);
      }

      void load(PixelT* pPixels, Vec2i size, bool takeData = false) {
        if (takeData) {
          setData(pPixels, size);
        }
        else {
          int64_t numPixels = size.x * size.y;
          m_pixels.pushBack(pPixels, pPixels + numPixels);
          m_size = size;
        }
      }

      Vec2i const& size() {
        return m_size;
      }

      PixelT* begin() {
        return m_pixels.begin();
      }

      PixelT* end() {
        return m_pixels.end();
      }

      PixelT const* begin() const {
        return m_pixels.begin();
      }

      PixelT const* end() const {
        return m_pixels.end();
      }

      PixelT* takeData(Vec2i* pSize, int64_t* pCapacity) {
        if (pSize != nullptr)
          *pSize = m_size;
        return m_pixels.takeData(0, pCapacity);
      }

      void setData(PixelT* pPixels, Vec2i size, int64_t capacity = -1) {
        m_size = size;
        m_pixels.setData(pPixels, size.x * size.y, capacity);
      }

      Surface getSurface() const {
        Surface s;
        s.format = FormatID;
        s.pitch = 0;
        s.size = Vec3i(m_size, 1);
        s.pBuffer = (void*)m_pixels.data();
        return s;
      }

    private:
      Vec2i m_size = Vec2i(0);
      Vector<PixelT> m_pixels;
    };
  }
}
