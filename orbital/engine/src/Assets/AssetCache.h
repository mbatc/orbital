#pragma once

#include "core/typeindex.h"
#include "core/URI.h"
#include "core/Stream.h"
#include <optional>

namespace engine {
  class AssetManager;
  class AssetLoadContext;
  class IAssetCache {
  public:
    /// The type of the asset loaded by this instance
    virtual bfc::type_index assetType() const = 0;
    /// Load the asset from the URI provided
    virtual bfc::Ref<void> _read(bfc::Stream *pStream) const = 0;
    /// Write the asset to `pStream`
    virtual bool _store(bfc::Ref<void> pAsset, bfc::Stream * pStream) const = 0;
  };

  template<typename T>
  class AssetCache : public IAssetCache {
  private:
    virtual bfc::type_index assetType() const override final {
      return bfc::TypeID<T>();
    }

    virtual bfc::Ref<void> _read(bfc::Stream* pStream) const override {
      return read(pStream);
    }

    virtual bool _store(bfc::Ref<void> pAsset, bfc::Stream * pStream) const override {
      return store(std::static_pointer_cast<T>(pAsset), pStream);
    }

  public:
    /// Load the asset from the URI provided
    virtual bfc::Ref<T> read(bfc::Stream * pStream) const = 0;
    /// Write the asset to `pStream`
    virtual bool store(bfc::Ref<T> pAsset, bfc::Stream * pStream) const = 0;
  };
}
