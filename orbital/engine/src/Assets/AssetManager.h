#pragma once

#include "Subsystem.h"
#include "VirtualFileSystem.h"

#include "core/Map.h"
#include "core/Set.h"
#include "core/URI.h"
#include "util/ThreadPool.h"
#include "util/UUID.h"
#include "util/Cache.h"

#include <limits>
#include <mutex>
#include <optional>

namespace engine {
  using namespace bfc;

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
  class IAssetCache;

  class AssetLoadContext {
  public:
    std::mutex               lock;
    bfc::Vector<AssetHandle> dependencies;
  };

  namespace asset_manager {
    bool init(Application * pApp);
    void update();
    void shutdown();

    /// Register an asset loader with the asset manager.
    bool registerLoader(bfc::StringView const & name, bfc::Ref<IAssetLoader> const & pLoader);

    /// Register an asset cache with the asset manager.
    bool registerCache(bfc::Ref<IAssetCache> const & pCache);

    /// Check if an asset is contained in the asset manager by its handle.
    bool contains(AssetHandle const & handle);

    /// Check if an asset is contained in the asset manager by its ID.
    bool contains(bfc::UUID const & uuid);

    /// Find the handle of an asset by its uuid.
    AssetHandle find(bfc::UUID const & uuid);

    /// Find the handle of an asset by its type and uri.
    // template<typename T>
    // AssetHandle find(bfc::URI const & uri) {}

    /// Find the handle of an asset by its pointer.
    AssetHandle find(bfc::Ref<void> const & pAsset);

    /// Add an asset to the asset manager.
    template<typename T>
    AssetHandle add(bfc::URI const & uri) {
      return add(uri, bfc::TypeID<T>());
    }

    /// Add an asset to the asset manager by its type.
    AssetHandle add(bfc::URI const & uri, bfc::type_index const & type);

    /// Add an asset to the asset manager, specifying a loader type.
    AssetHandle add(bfc::URI const & uri, bfc::StringView const & loaderID);

    bfc::UUID uuidOf(AssetHandle const & handle);

    bfc::URI uriOf(AssetHandle const & handle);

    /// Load the asset associated with `uri`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(bfc::URI const & uri, uint64_t * pLoadedVersion = nullptr, AssetLoadContext * pContext = nullptr) {
      return load<T>(add<T>(uri), pLoadedVersion, pContext);
    }

    /// Load the asset associated with `uri`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(bfc::UUID const & uuid, uint64_t * pLoadedVersion = nullptr, AssetLoadContext * pContext = nullptr) {
      return load<T>(find(uuid), pLoadedVersion);
    }

    /// Load the asset associated with `handle`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(AssetHandle const & handle, uint64_t * pLoadedVersion = nullptr,
                     AssetLoadContext * pContext = nullptr) {
      return std::static_pointer_cast<T>(load(handle, bfc::TypeID<T>(), pLoadedVersion));
    }

    /// Get the asset associated with `handle`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    bfc::Ref<void> load(AssetHandle const & handle, std::optional<bfc::type_index> const & type = std::nullopt,
                        uint64_t * pLoadedVersion = nullptr, AssetLoadContext * pContext = nullptr);

    template<typename T>
    std::future<bfc::Ref<T>> loadAsync(bfc::URI const & uri, AssetLoadContext * pContext = nullptr) {
      return bfc::async([=]() { return load(uri, pContext); });
    }

    template<typename T>
    std::future<bfc::Ref<T>> loadAsync(bfc::UUID const & uuid, AssetLoadContext * pContext = nullptr) {
      return bfc::async([=]() { return load(uuid, pContext); });
    }

    template<typename T>
    std::future<bfc::Ref<T>> loadAsync(AssetHandle const & handle, AssetLoadContext * pContext = nullptr) {
      return bfc::async([=]() { return load(handle, pContext); });
    }

    std::future<bfc::Ref<void>> loadAsync(AssetHandle const &                    handle,
                                          std::optional<bfc::type_index> const & type = std::nullopt,
                                          uint64_t * pLoadedVersion = nullptr, AssetLoadContext * pContext = nullptr);

    /// Get a reference to the asset version.
    bfc::Ref<uint64_t> getVersionReference(AssetHandle const & handle);

    /// Find the ID of a loader that can handle the URI and asset `type` specified.
    std::optional<bfc::String> findLoaderID(bfc::URI const & uri, bfc::type_index const & type);

    bool canLoad(bfc::URI const & uri, bfc::type_index const & type);

    template<typename T>
    bool canLoad(bfc::URI const & uri) {
      return canLoad(uri, bfc::TypeID<T>());
    }

    /// Reload the asset associated with `handle`.
    bool reload(AssetHandle const & handle);

    template<typename T>
    bfc::Vector<AssetHandle>
    findHandles(std::function<bool(bfc::URI const & uri, bfc::StringView const & loaderID)> const & filter = nullptr) {
      return findHandles([=](bfc::URI const & uri, bfc::type_index const & type, bfc::StringView const & loaderID) {
        return type == bfc::TypeID<T>() && (filter == nullptr || filter(uri, loaderID));
      });
    }

    bfc::Vector<AssetHandle> findHandles(
      std::function<bool(bfc::URI const & uri, bfc::type_index const & type, bfc::StringView const & loaderID)> const &
        filter = nullptr);
  }

  inline uint64_t hash(AssetHandle const& o) {
    return o.index;
  }

  /// An asset reference.
  template<typename T>
  class Asset
  {
  public:
    Asset() = default;

    Asset(std::nullptr_t const &) {}

    Asset(bfc::Ref<T> const & pAsset)
      : m_pInstance(pAsset) {
      m_handle          = asset_manager::find(pAsset);
      m_pManagedVersion = asset_manager::getVersionReference(m_handle);
    }

    Asset(AssetHandle const & handle)
      : m_handle(handle)
      , m_pManagedVersion(asset_manager::getVersionReference(handle)) {}

    Asset(bfc::URI const & uri)
      : Asset(asset_manager::add<T>(uri))
    {}

    static Asset<T> create(bfc::URI const & uri) {
      return Asset<T>(uri);
    }

    bool assign(bfc::URI const & uri) {
      m_handle          = asset_manager::add<T>(uri);
      m_pManagedVersion = asset_manager::getVersionReference(m_handle);

      return m_handle != InvalidAssetHandle;
    }

    bool assign(bfc::Ref<T> const & pInstance) {
      m_handle          = asset_manager::find(pInstance);
      m_pManagedVersion = asset_manager::getVersionReference(m_handle);
      m_pInstance       = pInstance;
      return true;
    }

    bool assign(AssetHandle const & handle) {
      m_handle          = handle;
      m_pManagedVersion = asset_manager::getVersionReference(handle);

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
      if (m_handle != InvalidAssetHandle) {
        // Check it is not out dated.
        if (*m_pManagedVersion != m_loadedVersion) {
          m_pInstance = asset_manager::load<T>(m_handle, &m_loadedVersion);
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

    bool operator==(Asset const & rhs) const {
      return (m_handle != InvalidAssetHandle && m_handle == rhs.m_handle) || m_pInstance == rhs.m_pInstance;
    }

    bool operator!=(Asset const & rhs) const {
      return !(*this == rhs);
    }

  private:
    mutable bfc::Ref<T> m_pInstance       = nullptr;
    mutable uint64_t    m_loadedVersion   = 0;
    bfc::Ref<uint64_t>  m_pManagedVersion = nullptr;
    AssetHandle         m_handle          = InvalidAssetHandle;
  };
} // namespace engine

namespace bfc {
  template<typename T>
  struct Serializer<engine::Asset<T>> {
    template<typename Ctx>
    static bfc::SerializedObject write(engine::Asset<T> const & o, Ctx const & ctx) {
      engine::AssetHandle    handle  = o.handle();
      if (handle == engine::InvalidAssetHandle) {
        return SerializedObject::Empty();
      }

      return SerializedObject::MakeMap({{"uri", engine::asset_manager::uriOf(handle).str()}});
    }

    template<typename Ctx>
    static bool read(bfc::SerializedObject const & s, engine::Asset<T> & o, Ctx const & ctx) {
      SerializedObject const & uriItem = s.get("uri");
      if (uriItem.isText()) {
        bfc::URI uri = uriItem.asText();
        mem::construct(&o, uri);
        return true;
      }

      SerializedObject const & idItem = s.get("uuid");
      if (idItem.isText()) {
        bfc::UUID uuid = idItem.asText();
        mem::construct(&o, engine::asset_manager::find(uuid));
      }

      mem::construct(&o, nullptr);
      return true;
    }
  };
}
