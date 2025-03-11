#pragma once

#include "AssetLoader.h"
#include "render/GraphicsDevice.h"

namespace bfc {
  namespace media {
    class Surface;
  }
} // namespace bfc

namespace engine {
  class SurfaceLoader : public AssetLoader<bfc::media::Surface> {
  public:
    virtual bfc::Ref<bfc::media::Surface> load(bfc::URI const & uri, AssetLoadContext * pManager) const override;
    virtual bool                          handles(bfc::URI const & uri, AssetManager const * pManager) const override;
  };

  class Texture2DLoader : public AssetLoader<bfc::graphics::Texture> {
  public:
    Texture2DLoader(bfc::GraphicsDevice * pGraphicsDevice);

    virtual bfc::Ref<bfc::graphics::Texture> load(bfc::URI const & uri, AssetLoadContext * pManager) const override;
    virtual bool                 handles(bfc::URI const & uri, AssetManager const * pManager) const override;

  private:
    bfc::GraphicsDevice * m_pGraphicsDevice = nullptr;
  };
} // namespace engine
