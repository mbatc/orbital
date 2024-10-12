#include "render/Texture.h"
#include "media/Image.h"
#include "media/Surface.h"

namespace bfc {
  bool Texture::load2D(GraphicsDevice * pDevice, URI const & path) {
    media::Surface surface;
    if (!loadSurface(path, &surface)) {
      return false;
    }
    load2D(pDevice, surface);
    return true;
  }

  Texture::Texture(GraphicsDevice * pDevice, media::Surface const & surface) {
    load2D(pDevice, surface);
  }

  Texture::Texture(GraphicsDevice * pDevice, Vec2i const & size, PixelFormat const & format, void const * pPixels, int64_t rowPitch) {
    load2D(pDevice, size, format, pPixels, rowPitch);
  }

  Texture::Texture(GraphicsDevice * pDevice, Vec2i const & size, DepthStencilFormat depthFormat) {
    load2D(pDevice, size, depthFormat);
  }

  void Texture::load2D(GraphicsDevice * pDevice, media::Surface const & surface) {
    upload(pDevice, TextureType_2D, surface);
  }

  void Texture::load2D(GraphicsDevice * pDevice, Vec2i const & size, PixelFormat const & format, void const * pPixels, int64_t rowPitch) {
    upload(pDevice, TextureType_2D, {size, 1}, format, pPixels, rowPitch);
  }

  void Texture::load2D(GraphicsDevice * pDevice, Vec2i const & size, DepthStencilFormat const & depthFormat) {
    upload(pDevice, TextureType_2D, {size, 1}, depthFormat);
  }

  bool Texture::loadSub2D(GraphicsDevice * pDevice, media::Surface const & surface, Vec2i offset) {
    graphics::TextureManager * pTextures = pDevice->getTextureManager();
    GraphicsResource           resource  = getResource();
    if (resource == InvalidGraphicsResource)
      return false;
    pTextures->uploadSubData(resource, surface, {offset, 0});
    return true;
  }

  void Texture::load2DArray(GraphicsDevice * pDevice, media::Surface const & surface) {
    upload(pDevice, TextureType_2DArray, surface);
  }

  void Texture::load2DArray(GraphicsDevice * pDevice, Vec3i const & size, PixelFormat const & format, void const * pPixels, int64_t rowPitch) {
    upload(pDevice, TextureType_2DArray, size, format, pPixels, rowPitch);
  }

  void Texture::load2DArray(GraphicsDevice * pDevice, Vec3i const & size, DepthStencilFormat const & depthFormat) {
    upload(pDevice, TextureType_2DArray, size, depthFormat);
  }

  void Texture::load3D(GraphicsDevice * pDevice, media::Surface const & surface) {
    upload(pDevice, TextureType_3D, surface);
  }

  void Texture::load3D(GraphicsDevice * pDevice, Vec3i const & size, PixelFormat const & format, void const * pPixels, int64_t rowPitch) {
    upload(pDevice, TextureType_3D, size, format, pPixels, rowPitch);
  }

  void Texture::load3D(GraphicsDevice * pDevice, Vec3i const & size, DepthStencilFormat const & depthFormat) {
    upload(pDevice, TextureType_3D, size, depthFormat);
  }

  void Texture::loadCubeMap(GraphicsDevice * pDevice, media::Surface const & surface) {
    BFC_ASSERT(surface.size.z == CubeMapFace_Count, "Surface must have a depth of 6");

    upload(pDevice, TextureType_CubeMap, surface);
  }

  void Texture::loadCubeMap(GraphicsDevice * pDevice, Vec2i const & size, PixelFormat const & format, void const * pPixels, int64_t rowPitch) {
    upload(pDevice, TextureType_CubeMap, {size, CubeMapFace_Count}, format, pPixels, rowPitch);
  }

  void Texture::loadCubeMap(GraphicsDevice * pDevice, Vec2i const & size, DepthStencilFormat const & depthFormat) {
    upload(pDevice, TextureType_CubeMap, {size, CubeMapFace_Count}, depthFormat);
  }

  void Texture::upload(GraphicsDevice * pDevice, TextureType const & type, media::Surface const & surface) {
    graphics::TextureManager * pTextures = pDevice->getTextureManager();
    GraphicsResource           resource  = getResource();
    if (getTextureType() != type)
      resource = pTextures->createTexture(type);
    pTextures->upload(resource, surface);
    set(pDevice, resource);
  }

  void Texture::upload(GraphicsDevice * pDevice, TextureType const & type, Vec3i const & size, PixelFormat const & format, void const * pPixels,
                       int64_t rowPitch) {
    media::Surface surface;
    surface.pBuffer = (void *)pPixels;
    surface.format  = format;
    surface.size    = size;
    surface.pitch   = rowPitch;
    return upload(pDevice, type, surface);
  }

  void Texture::upload(GraphicsDevice * pDevice, TextureType const & type, Vec3i const & size, DepthStencilFormat const & depthFormat) {
    graphics::TextureManager * pTextures = pDevice->getTextureManager();
    GraphicsResource           resource  = getResource();
    if (getTextureType() != type)
      resource = pTextures->createTexture(type);
    pTextures->upload(resource, depthFormat, size);
    set(pDevice, resource);
  }

  void Texture::generateMipmaps() {
    getDevice()->getTextureManager()->generateMipMaps(getResource());
  }

  bool Texture::isDepthTexture() const {
    return hasResource() ? getDevice()->getTextureManager()->isDepthTexture(getResource()) : TextureType_Unknown;
  }

  PixelFormat Texture::getPixelFormat() const {
    return hasResource() ? getDevice()->getTextureManager()->getColourFormat(getResource()) : PixelFormat_Unknown;
  }

  DepthStencilFormat Texture::getDepthStencilFormat() const {
    return hasResource() ? getDevice()->getTextureManager()->getDepthStencilFormat(getResource()) : DepthStencilFormat_Unknown;
  }

  Vec2i Texture::getSize() const {
    return hasResource() ? getDevice()->getTextureManager()->getSize(getResource()) : Vec2i(0);
  }

  TextureType Texture::getTextureType() const {
    return hasResource() ? getDevice()->getTextureManager()->getType(getResource()) : TextureType_Unknown;
  }
} // namespace bfc
