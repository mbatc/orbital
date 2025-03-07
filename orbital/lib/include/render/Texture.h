#pragma once

#include "GraphicsDevice.h"

namespace bfc {
  enum DepthStencilFormat;
  class GraphicsDevice;
  class Filename;
  template<typename T>
  struct Colour;

  namespace media {
    class Surface;
    template<typename T>
    class Image;
  } // namespace media

  class BFC_API Texture {
  public:
    Texture() = default;
    Texture(GraphicsDevice * pDevice, media::Surface const & surface);
    Texture(GraphicsDevice * pDevice, Vec2i const & size, PixelFormat const & format, void const * pPixels = nullptr, int64_t rowPitch = 0);
    Texture(GraphicsDevice * pDevice, Vec2i const & size, DepthStencilFormat depthFormat);

    template<typename Format>
    Texture(GraphicsDevice * pDevice, media::Image<Format> const & image) {
      load2D(pDevice, image.getSurface());
    }

    template<typename Format>
    Texture(GraphicsDevice * pDevice, Vec2i const & size, Colour<Format> * pPixels) {
      load2D(pDevice, size, pPixels);
    }

    template<typename Format>
    bool load2D(GraphicsDevice * pDevice, URI const & path) {
      media::Image<Format> img;
      if (!img.load(path)) {
        return false;
      }
      load2D(pDevice, img);
      return true;
    }

    bool load2D(GraphicsDevice * pDevice, URI const & path);
    void load2D(GraphicsDevice * pDevice, media::Surface const & surface);
    void load2D(GraphicsDevice * pDevice, Vec2i const & size, PixelFormat const & format, void const * pPixels = nullptr, int64_t rowPitch = 0);
    void load2D(GraphicsDevice * pDevice, Vec2i const & size, DepthStencilFormat const & depthFormat);
    template<typename Format>
    void load2D(GraphicsDevice * pDevice, media::Image<Format> const & image) {
      load2D(pDevice, image.getSurface());
    }
    template<typename Format>
    void load2D(GraphicsDevice * pDevice, Vec2i const & size, Colour<Format> * pPixels = nullptr) {
      load2D(pDevice, size, Format::FormatID, pPixels);
    }
    bool loadSub2D(GraphicsDevice * pDevice, media::Surface const & surface, Vec2i offset);

    void load2DArray(GraphicsDevice * pDevice, media::Surface const & surface);
    void load2DArray(GraphicsDevice * pDevice, Vec3i const & size, PixelFormat const & format, void const * pPixels = nullptr, int64_t rowPitch = 0);
    void load2DArray(GraphicsDevice * pDevice, Vec3i const & size, DepthStencilFormat const & depthFormat);
    template<typename Format>
    void load2DArray(GraphicsDevice * pDevice, Vec2i const & size, Colour<Format> * pPixels = nullptr) {
      load2DArray(pDevice, size, Format::FormatID, pPixels);
    }

    void load3D(GraphicsDevice * pDevice, media::Surface const & surface);
    void load3D(GraphicsDevice * pDevice, Vec3i const & size, PixelFormat const & format, void const * pPixels = nullptr, int64_t rowPitch = 0);
    void load3D(GraphicsDevice * pDevice, Vec3i const & size, DepthStencilFormat const & depthFormat);

    template<typename Format>
    void load3D(GraphicsDevice * pDevice, Vec3i const & size, Colour<Format> * pPixels = nullptr) {
      load3D(pDevice, size, Format::FormatID, pPixels);
    }

    void loadCubeMap(GraphicsDevice * pDevice, media::Surface const & surface);
    void loadCubeMap(GraphicsDevice * pDevice, Vec2i const & size, PixelFormat const & format, void const * pPixels = nullptr, int64_t rowPitch = 0);
    void loadCubeMap(GraphicsDevice * pDevice, Vec2i const & size, DepthStencilFormat const & depthFormat);

    template<typename Format>
    void loadCubeMap(GraphicsDevice * pDevice, Vec2i const & size, Colour<Format> * pPixels = nullptr) {
      loadCubeMap(pDevice, size, Format::FormatID, pPixels);
    }

    void upload(GraphicsDevice * pDevice, TextureType const & type, media::Surface const & surface);
    void upload(GraphicsDevice * pDevice, TextureType const & type, Vec3i const & size, PixelFormat const & format, void const * pPixels = nullptr,
                int64_t rowPitch = 0);
    void upload(GraphicsDevice * pDevice, TextureType const & type, Vec3i const & size, DepthStencilFormat const & depthFormat);

    void generateMipmaps();

    bool isDepthTexture() const;

    PixelFormat getPixelFormat() const;

    DepthStencilFormat getDepthStencilFormat() const;

    Vec2i getSize() const;

    TextureType getTextureType() const;
  };
} // namespace bfc
