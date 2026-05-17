#pragma once

#include "AssetLoader.h"
#include "mesh/MeshData.h"

namespace bfc {
  class GraphicsDevice;
  class Material;
} // namespace bfc

namespace engine {
  class MaterialFileLoader : public AssetLoader<bfc::MeshData::Material> {
  public:
    virtual bfc::Ref<bfc::MeshData::Material> load(bfc::URI const & uri, AssetLoadContext * pManager) const override;
    virtual bool                              handles(bfc::URI const & uri, AssetManager const * pManager) const override;
  };

  class MaterialLoader : public AssetLoader<bfc::Material> {
  public:
    MaterialLoader(bfc::GraphicsDevice * pDevice);

    virtual bfc::Ref<bfc::Material> load(bfc::URI const & uri, AssetLoadContext * pManager) const override;
    virtual bool                    handles(bfc::URI const & uri, AssetManager const * pManager) const override;

  private:
    bfc::GraphicsDevice * m_pGraphics;
  };

} // namespace engine
