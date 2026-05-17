#pragma once

#include "core/typeindex.h"
#include "core/URI.h"

namespace engine {
  class AssetManager;
  class AssetLoadContext;
  class IAssetLoader {
  public:
    /// The type of the asset loaded by this instance.
    virtual bfc::type_index assetType() const = 0;

    /// Load the asset from the URI provided.
    virtual bfc::Ref<void> loadUnknown(bfc::URI const & uri, AssetLoadContext * pContext) const = 0;

    /// Query if this loader can handle the URI provided.
    virtual bool handles(bfc::URI const & uri, AssetManager const * pManager) const = 0;
  };

  template<typename T>
  class AssetLoader : public IAssetLoader {
  private:
    virtual bfc::type_index assetType() const override final {
      return bfc::TypeID<T>();
    }

    virtual bfc::Ref<void> loadUnknown(bfc::URI const & uri, AssetLoadContext * pContext) const override final {
      return load(uri, pContext);
    }

  public:
    virtual bfc::Ref<T> load(bfc::URI const & uri, AssetLoadContext * pContext) const = 0;
  };
}
