#pragma once

#include "Subsystem.h"
#include "VirtualFileSystem.h"

#include "core/Map.h"
#include "core/Set.h"
#include "core/URI.h"
#include "util/UUID.h"

#include <limits>
#include <mutex>
#include <optional>

namespace engine {
  struct AssetHandle {
    uint64_t index = std::numeric_limits<uint64_t>::max();

    operator uint64_t() const {
      return index;
    }
  };

  constexpr AssetHandle InvalidAssetHandle = {};

  enum AssetStatus {
    AssetStatus_Unloaded,
    AssetStatus_Loading,
    AssetStatus_Loaded,
    AssetStatus_Failed,
  };

  class IAssetLoader;
  class AssetManager : public Subsystem {
  public:
    AssetManager();

    bool init(Application *pApp) override;

    /// Register an asset loader with the asset manager.
    bool registerLoader(bfc::StringView const & name, bfc::Ref<IAssetLoader> const & pLoader);

    /// Check if an asset is contained in the asset manager by its handle.
    bool contains(AssetHandle const & handle) const;

    /// Check if an asset is contained in the asset manager by its ID.
    bool contains(bfc::UUID const & uuid) const;

    /// Find the handle of an asset by its uuid.
    AssetHandle find(bfc::UUID const & uuid) const;

    /// Find the handle of an asset by its type and uri.
    template<typename T>
    AssetHandle find(bfc::URI const & uri) const {}

    /// Find the handle of an asset by its pointer.
    AssetHandle find(bfc::Ref<void> const & pAsset) const;

    /// Add an asset to the asset manager.
    template<typename T>
    AssetHandle add(bfc::URI const & uri) {
      return add(uri, bfc::TypeID<T>());
    }

    /// Add an asset to the asset manager by its type.
    AssetHandle add(bfc::URI const & uri, bfc::type_index const & type);

    /// Add an asset to the asset manager, specifying a loader type.
    AssetHandle add(bfc::URI const & uri, bfc::StringView const & loaderID);

    bfc::UUID uuidOf(AssetHandle const & handle) const;

    bfc::URI uriOf(AssetHandle const & handle) const;

    /// Load the asset associated with `uri`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(bfc::URI const & uri) {
      return load<T>(add<T>(uri));
    }

    /// Load the asset associated with `uri`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(bfc::UUID const & uuid) {
      return load<T>(find(uuid));
    }

    /// Load the asset associated with `handle`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(AssetHandle const & handle) {
      return std::static_pointer_cast<T>(load(handle, bfc::TypeID<T>()));
    }

    /// Get the asset associated with `handle`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    bfc::Ref<void> load(AssetHandle const & handle, std::optional<bfc::type_index> const & type = std::nullopt);

    /// Find the ID of a loader that can handle the URI and asset `type` specified.
    std::optional<bfc::String> findLoaderID(bfc::URI const & uri, bfc::type_index const & type) const;

    template<typename T>
    bool canLoad(bfc::URI const & uri) const {
      return findLoaderID(uri, bfc::TypeID<T>()).has_value();
    }

    /// Reload the asset associated with `handle`.
    bool reload(AssetHandle const & handle);

    VirtualFileSystem * getFileSystem() const;

  private:
    bfc::Ref<IAssetLoader> findLoader_unlocked(bfc::StringView const & loaderID) const;
    bool                   reload(AssetHandle const & handle, std::unique_lock<std::mutex> &lock);

    struct Asset {
      AssetStatus     status = AssetStatus_Unloaded;
      bfc::UUID       uuid;
      bfc::URI        uri;
      bfc::Ref<void>  pInstance;
      bfc::type_index type = bfc::TypeID<void>();
      bfc::String     loader;

      uint64_t version           = 1;
      uint64_t lastVersionLoaded = 0;

      bfc::Set<AssetHandle> dependent;
    };

    struct LoaderInfo {
      bfc::String            loaderID;
      bfc::Ref<IAssetLoader> pLoader;
    };

    mutable std::mutex      m_loaderLock;
    bfc::Vector<LoaderInfo> m_loaders;

    mutable std::mutex              m_assetLock;
    mutable std::condition_variable m_assetNotifier;

    bfc::Pool<Asset>                 m_assetPool;
    bfc::Map<bfc::UUID, AssetHandle> m_idToHandle;
    bfc::Map<void*, AssetHandle>     m_ptrToHandle;

    bfc::Ref<VirtualFileSystem> m_pFileSystem;
  };

  inline uint64_t hash(AssetHandle const& o) {
    return o.index;
  }
} // namespace engine
