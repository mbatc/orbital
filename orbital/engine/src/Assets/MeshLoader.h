#pragma once

#include "AssetLoader.h"
#include "AssetCache.h"

namespace bfc {
  class GraphicsDevice;
  class MeshData;
  class Mesh;
  class Material;
}

namespace engine {
  class MeshDataFileLoader : public AssetLoader<bfc::MeshData> {
  public:
    virtual bfc::Ref<bfc::MeshData> load(bfc::URI const & uri, AssetLoadContext * pManager) const override;
    virtual bool                    handles(bfc::URI const & uri, AssetManager const * pManager) const override;
  };

  class MeshLoader : public AssetLoader<bfc::Mesh> {
  public:
    MeshLoader(bfc::GraphicsDevice *pDevice);

    virtual bfc::Ref<bfc::Mesh> load(bfc::URI const & uri, AssetLoadContext * pManager) const override;
    virtual bool                handles(bfc::URI const & uri, AssetManager const * pManager) const override;

  private:
    bfc::GraphicsDevice * m_pGraphics;
  };

  class MeshMaterialLoader : public AssetLoader<bfc::Material> {
  public:
    MeshMaterialLoader(bfc::GraphicsDevice * pDevice);

    virtual bfc::Ref<bfc::Material> load(bfc::URI const & uri, AssetLoadContext * pManager) const override;
    virtual bool                    handles(bfc::URI const & uri, AssetManager const * pManager) const override;

    inline static bfc::String fragmentPrefix = "material.";

  private:
    bfc::GraphicsDevice * m_pGraphics;
  };

  class MeshDataCache : public AssetCache<bfc::MeshData> {
  public:
    /// Load the asset from the URI provided
    virtual bfc::Ref<bfc::MeshData> read(bfc::Stream * pStream) const override;
    /// Write the asset to `pStream`
    virtual bool store(bfc::Ref<bfc::MeshData> pAsset, bfc::Stream * pStream) const override;
  };
} // namespace engine
