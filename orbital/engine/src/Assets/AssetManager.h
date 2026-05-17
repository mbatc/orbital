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

  template<typename T>
  class Asset;

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
    bfc::Ref<T> load(bfc::URI const & uri, uint64_t * pLoadedVersion = nullptr) {
      return load<T>(add<T>(uri), pLoadedVersion);
    }

    /// Load the asset associated with `uri`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(bfc::UUID const & uuid, uint64_t * pLoadedVersion = nullptr) {
      return load<T>(find(uuid), pLoadedVersion);
    }

    /// Load the asset associated with `handle`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(AssetHandle const & handle, uint64_t * pLoadedVersion = nullptr) {
      return std::static_pointer_cast<T>(load(handle, bfc::TypeID<T>(), pLoadedVersion));
    }

    /// Get the asset associated with `handle`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    bfc::Ref<void> load(AssetHandle const & handle, std::optional<bfc::type_index> const & type = std::nullopt, uint64_t * pLoadedVersion = nullptr);

    /// Get a reference to the asset version.
    bfc::Ref<uint64_t> getVersionReference(AssetHandle const & handle) const;

    /// Find the ID of a loader that can handle the URI and asset `type` specified.
    std::optional<bfc::String> findLoaderID(bfc::URI const & uri, bfc::type_index const & type) const;

    bool canLoad(bfc::URI const & uri, bfc::type_index const & type) const;

    template<typename T>
    bool canLoad(bfc::URI const & uri) const {
      return canLoad(uri, bfc::TypeID<T>());
    }

    /// Reload the asset associated with `handle`.
    bool reload(AssetHandle const & handle);

    VirtualFileSystem * getFileSystem() const;

    template<typename T>
    bfc::Vector<AssetHandle> findHandles(std::function<bool(bfc::URI const & uri, bfc::StringView const & loaderID)> const & filter = nullptr) const {
      return findHandles([=](bfc::URI const & uri, bfc::type_index const & type, bfc::StringView const & loaderID) {
        return type == bfc::TypeID<T>() && (filter == nullptr || filter(uri, loaderID));
      });
    }

    bfc::Vector<AssetHandle>
    findHandles(std::function<bool(bfc::URI const & uri, bfc::type_index const & type, bfc::StringView const & loaderID)> const & filter = nullptr) const;

    virtual void loop(Application * pApp) override;

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
      bfc::Ref<uint64_t> version = nullptr;
      uint64_t lastVersionLoaded = 0;

      bfc::Vector<bfc::Pair<uint64_t *, bfc::Ref<void> *>> waiting; ///< Waiting for the load to complete

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

  /// An asset reference.
  template<typename T>
  class Asset
  {
  public:
    Asset() = default;

    Asset(bfc::Ref<T> const & pAsset)
      : m_pInstance(pAsset) {}

    Asset(AssetManager * pManager, bfc::Ref<T> const & pAsset)
      : m_pInstance(pAsset)
      , m_pManager(pManager) {
      m_handle          = pManager->find(pAsset);
      m_pManagedVersion = pManager->getVersionReference(m_handle);
    }

    Asset(AssetManager * pManager, AssetHandle const & handle)
      : m_pManager(pManager)
      , m_handle(handle)
      , m_pManagedVersion(pManager->getVersionReference(handle)) {}

    Asset(AssetManager * pManager, bfc::URI const & uri)
      : Asset(pManager, pManager->add<T>(uri))
    {}

    static Asset<T> create(AssetManager * pManager, bfc::URI const & uri) {
      return Asset<T>(pManager, uri);
    }

    bool assign(AssetManager * pManager, bfc::URI const & uri) {
      m_pManager        = pManager;
      m_handle          = pManager->add<T>(uri);
      m_pManagedVersion = m_pManager == nullptr ? nullptr : pManager->getVersionReference(m_handle);

      return m_handle != InvalidAssetHandle;
    }

    bool assign(AssetManager * pManager, bfc::Ref<T> const & pInstance) {
      m_handle          = pManager->find(pInstance);
      m_pManagedVersion = pManager->getVersionReference(m_handle);
      m_pInstance       = pInstance;
      return true;
    }

    bool assign(AssetManager * pManager, AssetHandle const & handle) {
      m_pManager        = pManager;
      m_handle          = handle;
      m_pManagedVersion = m_pManager == nullptr ? nullptr : pManager->getVersionReference(handle);

      return m_handle != InvalidAssetHandle;
    }

    /// The reference has an asset instance.
    bool loaded() const {
      return m_pInstance != nullptr;
    }

    /// The managed asset is out of date.
    /// Likely been reloaded via the asset manager.
    bool expired() const {
      return *m_pManagedVersion != m_loadedVersion;
    }

    bfc::Ref<T> const& instance() const {
      if (m_handle != InvalidAssetHandle && m_pManager != nullptr) {
        // Check it is not out dated.
        if (*m_pManagedVersion != m_loadedVersion) {
          m_pInstance = m_pManager->load<T>(m_handle, &m_loadedVersion);
        }
      }

      return m_pInstance;
    }

    AssetHandle const& handle() const {
      return m_handle;
    }

    /// Implicit cast to Ref for easy integration with other APIs
    operator bfc::Ref<T> const &() const {
      return instance();
    }

    /// Implicit cast to ptr for easy integration with other APIs
    operator T* () const {
      return instance().get();
    }

    operator bool() const {
      return instance() != nullptr;
    }

    T& operator*() const {
      return *instance();
    }

    T* operator->() const {
      return instance().get();
    }

  private:
    mutable bfc::Ref<T> m_pInstance       = nullptr;
    mutable uint64_t    m_loadedVersion   = 0;
    AssetManager *      m_pManager        = nullptr;
    bfc::Ref<uint64_t>  m_pManagedVersion = nullptr;
    AssetHandle         m_handle          = InvalidAssetHandle;
  };

  struct AssetSerializerContext {
    AssetManager * pAssetManager;
  };
} // namespace engine

namespace bfc {
  template<typename T>
  struct Serializer<engine::Asset<T>> {
    static bfc::SerializedObject write(engine::Asset<T> const & o, engine::AssetSerializerContext const & ctx) {
      engine::AssetManager * pAssets = ctx.pAssetManager;
      engine::AssetHandle    handle  = o.handle();
      if (handle == engine::InvalidAssetHandle) {
        return SerializedObject::Empty();
      }

      return SerializedObject::MakeMap({{"uri", pAssets->uriOf(handle).str()}});
    }

    static bool read(bfc::SerializedObject const & s, engine::Asset<T> & o, engine::AssetSerializerContext const & ctx) {
      engine::AssetManager *   pAssets = ctx.pAssetManager;
      SerializedObject const & uriItem = s.get("uri");
      if (uriItem.isText()) {
        bfc::URI               uri     = uriItem.asText();
        mem::construct(&o, pAssets, uri);
        return true;
      }

      SerializedObject const & idItem = s.get("uuid");
      if (idItem.isText()) {
        bfc::UUID uuid = idItem.asText();
        mem::construct(&o, pAssets, pAssets->find(uuid));
      }

      mem::construct(&o, nullptr);
      return true;
    }
  };
}
