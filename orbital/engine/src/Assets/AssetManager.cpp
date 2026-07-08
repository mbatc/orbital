#include "AssetManager.h"
#include "AssetLoadContext.h"
#include "AssetLoader.h"
#include "AssetCache.h"
#include "Assets/MeshLoader.h"
#include "Assets/ShaderLoader.h"
#include "Assets/TextureLoader.h"
#include "Assets/SkyboxLoader.h"
#include "Assets/MaterialLoader.h"
#include "core/Stream.h"
#include "core/Serialize.h"

#include "Application.h"
#include "Rendering/Rendering.h"

#include "util/Log.h"

using namespace bfc;

namespace {
  struct CacheHeader {
    struct Dependency {
      URI       uri;
      Timestamp lastModified;
    };
    int32_t            version = 1;
    Vector<Dependency> dependencies; ///< URIs this cache entry depends on.
  };
} // namespace

namespace bfc {
  int64_t write(Stream * pStream, ::CacheHeader::Dependency const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].uri) && pStream->write(pValue[i].lastModified)))
        return i;
    }
    return count;
  }

  int64_t read(Stream * pStream, ::CacheHeader::Dependency * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].uri) && pStream->read(&pValue[i].lastModified)))
        return i;
    }
    return count;
  }
  int64_t write(Stream * pStream, ::CacheHeader const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].version)
        && pStream->write(pValue[i].dependencies)
        ))
        return i;
    }
    return count;
  }

  int64_t read(Stream * pStream, ::CacheHeader * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].version)
        && pStream->read(&pValue[i].dependencies)
        ))
        return i;
    }
    return count;
  }
}

namespace engine {
  AssetManager::AssetManager()
    : Subsystem(TypeID<AssetManager>(), "AssetManager") {}

  bool AssetManager::init(Application * pApp) {
    Ref<Rendering> pRendering = pApp->findSubsystem<Rendering>();

    m_pFileSystem = pApp->findSubsystem<VirtualFileSystem>();

    registerLoader("core.meshdata", NewRef<engine::MeshDataFileLoader>());
    registerLoader("core.mesh", NewRef<engine::MeshLoader>(pRendering->getDevice()));
    registerLoader("core.meshdata.material", NewRef<engine::MeshMaterialLoader>(pRendering->getDevice()));
    registerLoader("core.surface", NewRef<engine::SurfaceLoader>());
    registerLoader("core.texture", NewRef<engine::Texture2DLoader>(pRendering->getDevice()));
    registerLoader("core.shader", NewRef<engine::ShaderLoader>(pRendering->getDevice()));
    registerLoader("core.skybox", NewRef<engine::SkyboxLoader>(pRendering->getDevice()));
    registerLoader("core.materialdata", NewRef<engine::MaterialFileLoader>());
    registerLoader("core.material", NewRef<engine::MaterialLoader>(pRendering->getDevice()));
    registerCache(NewRef<engine::MeshDataCache>());
    registerCache(NewRef<engine::TextureCache>(pRendering->getDevice()));

    m_appDataPath = pApp->getAppDataPath() / "AssetManager";
    m_pCache      = bfc::NewRef<Cache>(m_appDataPath / "Cache");
    return true;
  }

  void AssetManager::shutdown() {
  }

  bool AssetManager::registerLoader(StringView const & name, Ref<IAssetLoader> const & pLoader) {
    std::scoped_lock guard{m_loaderLock};
    if (findLoader_unlocked(name) != nullptr) {
      return false;
    }

    m_loaders.pushBack({name, pLoader});

    return true;
  }

  bool AssetManager::registerCache(bfc::Ref<IAssetCache> const & pCache) {
    std::scoped_lock guard{m_loaderLock};
    if (findCache_unlocked(pCache->assetType()) != nullptr) {
      return false;
    }
    m_caches.pushBack(pCache);
    return true;
  }

  bool AssetManager::contains(AssetHandle const & handle) const {
    std::scoped_lock guard{m_assetLock};
    return m_assetPool.isUsed(handle);
  }

  bool AssetManager::contains(UUID const & uuid) const {
    std::scoped_lock guard{m_assetLock};
    return m_idToHandle.contains(uuid);
  }

  AssetHandle AssetManager::find(UUID const & uuid) const {
    std::scoped_lock guard{m_assetLock};
    AssetHandle      ret;
    if (!m_idToHandle.tryGet(uuid, &ret)) {
      return {};
    }
    return ret;
  }

  AssetHandle AssetManager::find(bfc::Ref<void> const & pAsset) const {
    std::scoped_lock guard{m_assetLock};
    AssetHandle ret;
    if (!m_ptrToHandle.tryGet(pAsset.get(), &ret)) {
      return {};
    }
    return ret;
  }

  AssetHandle AssetManager::add(URI const & uri, type_index const & type) {
    // Find a loader that can handle this asset.
    std::optional<String> loaderID = findLoaderID(uri, type);

    if (!loaderID.has_value()) {
      return {};
    }

    return add(uri, loaderID.value());
  }

  AssetHandle AssetManager::add(URI const & uri, StringView const & loaderID) {
    if (uri.path().empty()) {
      return {};
    }
    
    std::unique_lock loaderGuard {m_loaderLock};
    bfc::Ref<IAssetLoader> pLoader = findLoader_unlocked(loaderID);
    loaderGuard.unlock();

    std::scoped_lock guard{m_assetLock};
    for (int64_t index = 0; index < m_assetPool.capacity(); ++index) {
      if (!m_assetPool.isUsed(index)) {
        continue;
      }
      Asset & asset = m_assetPool[index];
      if (asset.loader == loaderID && asset.uri == uri) {
        return {(uint64_t)index};
      }
    }

    Asset newAsset;
    newAsset.loader  = loaderID;
    newAsset.uri     = uri;
    newAsset.uuid    = UUID::New();
    newAsset.version = bfc::NewRef<uint64_t>(1);
    newAsset.type    = pLoader->assetType();

    int64_t index = m_assetPool.emplace(newAsset);
    return {(uint64_t)index};
  }

  bfc::UUID AssetManager::uuidOf(AssetHandle const & handle) const {
    std::unique_lock assetGuard{m_assetLock};
    if (!m_assetPool.isUsed(handle)) {
      return {};
    }

    return m_assetPool[handle].uuid;
  }

  bfc::URI AssetManager::uriOf(AssetHandle const & handle) const {
    std::unique_lock assetGuard{m_assetLock};
    if (!m_assetPool.isUsed(handle)) {
      return {};
    }
    return m_assetPool[handle].uri;
  }

  Ref<void> AssetManager::load(AssetHandle const & handle, std::optional<type_index> const & type, uint64_t * pLoadedVersion) {
    bool     load    = false;
    bool     wait    = false;
    uint64_t loadingVersion = 0;

    Ref<IAssetLoader> pLoader = nullptr;
    Ref<IAssetCache>  pCache = nullptr;
    std::unique_lock  assetGuard{m_assetLock};
    {
      if (!m_assetPool.isUsed(handle)) {
        return nullptr;
      }

      Asset & stored = m_assetPool[handle];

      {
        std::scoped_lock guard{m_loaderLock};
        pLoader = findLoader_unlocked(stored.loader);
        pCache  = findCache_unlocked(pLoader->assetType());
      }

      if (pLoader->assetType() != type) {
        return nullptr;
      }

      Ref<void> pLoaded = stored.pInstance;
      if (pLoaded != nullptr) {
        if (pLoadedVersion != nullptr)
          *pLoadedVersion = stored.lastVersionLoaded;

        return pLoaded;
      }

      switch (stored.status) {
      case AssetStatus_Unloaded: load = true; break;
      case AssetStatus_Loading: wait = true; break;
      case AssetStatus_Failed:
        // only load if there is a new version of the asset.
        load = stored.lastVersionLoaded != *stored.version;
        loadingVersion = *stored.version;
        break;
      }

      if (load) {
        stored.status = AssetStatus_Loading;
      } else if (wait) {
        m_assetNotifier.wait(assetGuard, [&]() { return m_assetPool[handle].status != AssetStatus_Loading; });
      } else {
        return nullptr;
      }
    }

    URI assetUri = m_assetPool[handle].uri;

    Ref<void> pInstance;
    if (load) {
      assetGuard.unlock();

      if (pLoader == nullptr) {
        return nullptr;
      }

      AssetLoadContext context{this};
      auto             lastModified    = m_pFileSystem->lastModified(assetUri);
      if (lastModified.has_value()) {
        if (pCache != nullptr) {
          Cache::Entry entry;
          if (m_pCache->checkout(assetUri.c_str(), &entry)) {
            BFC_LOG_INFO("AssetManager", "Reading cached asset (handle: %lld, uri: %s, type: %s)",
                            handle.index, assetUri, pLoader->assetType().name());

            auto header = bfc::read<::CacheHeader>(&entry.stream);
            if (header.has_value()) {
              const bool isStale = header->dependencies.find([&](::CacheHeader::Dependency const & dep) {
                return m_pFileSystem->lastModified(dep.uri) != dep.lastModified;
              }) != -1;

              if (!isStale) {
                pInstance = pCache->_read(&entry.stream);
              } else {
                entry.stream.close();

                m_pCache->remove(assetUri.c_str());
              }
            }
          }
        }
      }

      bool loadedFromCache = pInstance != nullptr;
      if (!loadedFromCache) {
        pInstance = pLoader->_load(assetUri, &context);
      }

      assetGuard.lock();
      Asset & stored           = m_assetPool[handle];
      stored.lastVersionLoaded = loadingVersion;
      stored.pInstance         = pInstance;
      stored.status            = pInstance == nullptr ? AssetStatus_Failed : AssetStatus_Loaded;
      stored.lastModified      = lastModified;

      bool canTryCache = !loadedFromCache && lastModified.has_value();
      for (AssetHandle const & dependency : context.getDependencies()) {
        m_assetPool[dependency].dependent.add(handle);
        canTryCache &= m_assetPool[dependency].lastModified.has_value();
      }

      if (pInstance != nullptr) {
        BFC_LOG_INFO("AssetManager", "Loaded asset (handle: %lld, uri: %s, type: %s, loader: %s)", handle.index, stored.uri, pLoader->assetType().name(),
                     stored.loader);
        m_ptrToHandle.add(pInstance.get(), handle);
      } else {
        BFC_LOG_WARNING("AssetManager", "Failed to load asset (handle: %lld, uri: %s, type: %s, loader: %s)", handle.index, stored.uri,
                        pLoader->assetType().name(),
                        stored.loader);
      }

      if (pLoadedVersion != nullptr)
        *pLoadedVersion = loadingVersion;

      for (auto [pVersion, ppInstance] : stored.waiting) {
        *pVersion   = loadingVersion;
        *ppInstance = pInstance;
      }

      ::CacheHeader cacheHeader;
      cacheHeader.dependencies.pushBack({ assetUri, lastModified.value() });
      for (AssetHandle const & dependency : context.getDependencies())
        cacheHeader.dependencies.pushBack({ m_assetPool[dependency].uri, m_assetPool[dependency].lastModified.value() });
      assetGuard.unlock();
      m_assetNotifier.notify_all();

      if (pInstance != nullptr) {
        if (pCache != nullptr && canTryCache) {
          bfc::async([=, header = std::move(cacheHeader)]() {
            BFC_LOG_INFO("AssetManager", "Caching asset (handle: %lld, uri: %s, type: %s, loader: %s)", handle.index,
                         stored.uri, pLoader->assetType().name(), stored.loader);
            Cache::Entry newCacheEntry = m_pCache->create();
            newCacheEntry.stream.write(header);
            if (pCache->_store(pInstance, &newCacheEntry.stream))
              m_pCache->commit(assetUri.c_str(), &newCacheEntry);
            else
              BFC_LOG_WARNING("AssetManager", "Failed to write cache (handle: %lld, uri: %s, type: %s, loader: %s)",
                              handle.index, stored.uri, pLoader->assetType().name(), stored.loader);
          });
        }
      }
    } else if (wait) {
      // Another thread started loading this asset.
      // Wait for loading to finish.
      m_assetPool[handle].waiting.pushBack(Pair{&loadingVersion, &pInstance});

      m_assetNotifier.wait(assetGuard, [=, &pInstance, &loadingVersion]() {
        if (m_assetPool[handle].status == AssetStatus_Loading)
          return false;

        int64_t idx = m_assetPool[handle].waiting.find([pInstance](auto & o) { return o.second == &pInstance; });
        m_assetPool[handle].waiting.erase(idx);
        return true;
      });
    }

    return pInstance;
  }

  void AssetManager::loop(Application * pApp) {
    std::unique_lock assetGuard{m_assetLock};
    for (auto& asset : m_assetPool) {
      if (asset.pInstance.use_count() == 1) {
        m_ptrToHandle.erase(asset.pInstance.get());
        asset.pInstance = nullptr; // Remove the asset
        asset.status    = AssetStatus_Unloaded;
        *asset.version += 1;
        BFC_LOG_INFO("AssetManager", "Unloaded asset (uri: %s, type: %s, loader: %s)", asset.uri, asset.type.name(),
                     asset.loader);
      }
    }
  }

  bfc::Ref<uint64_t> AssetManager::getVersionReference(AssetHandle const & handle) const {
    std::unique_lock assetGuard{m_assetLock};

    return m_assetPool.isUsed(handle) ? m_assetPool[handle].version : nullptr;
  }

  std::optional<String> AssetManager::findLoaderID(URI const & uri, type_index const & type) const {
    Vector<LoaderInfo> loaders;
    {
      std::scoped_lock loaderLock{m_loaderLock};
      loaders = m_loaders;
    }

    for (LoaderInfo const & info : loaders) {
      if (info.pLoader->assetType() == type && info.pLoader->handles(uri, this)) {
        return info.loaderID;
      }
    }
    return std::nullopt;
  }

  bool AssetManager::canLoad(bfc::URI const & uri, bfc::type_index const & type) const {
    return findLoaderID(uri, type).has_value();
  }

  bool AssetManager::reload(AssetHandle const & handle) {
    std::unique_lock assetGuard{m_assetLock};

    return reload(handle, assetGuard);
  }

  VirtualFileSystem* AssetManager::getFileSystem() const {
    return m_pFileSystem.get();
  }

  bfc::Vector<AssetHandle>
  AssetManager::findHandles(std::function<bool(bfc::URI const & uri, bfc::type_index const & type, bfc::StringView const & loaderID)> const & filter) const {
    std::unique_lock assetGuard{m_assetLock};
    bfc::Vector<AssetHandle> ret;
    for (int64_t handle = 0; handle < m_assetPool.capacity(); ++handle) {
      if (!m_assetPool.isUsed(handle)) {
        continue;
      }

      Asset const & asset = m_assetPool[handle];

      if (filter == nullptr || filter(asset.uri, asset.type, asset.loader))
        ret.pushBack(AssetHandle{ (uint64_t)handle });
    }

    return ret;
  }

  Ref<IAssetLoader> AssetManager::findLoader_unlocked(StringView const & loaderID) const {
    for (int64_t i = 0; i < m_loaders.size(); ++i) {
      if (m_loaders[i].loaderID == loaderID) {
        return m_loaders[i].pLoader;
      }
    }
    return nullptr;
  }

  bfc::Ref<IAssetCache> AssetManager::findCache_unlocked(bfc::type_index const & assetType) const {
    for (int64_t i = 0; i < m_caches.size(); ++i) {
      if (m_caches[i]->assetType() == assetType) {
        return m_caches[i];
      }
    }
    return nullptr;
  }

  bool AssetManager::reload(AssetHandle const & handle, std::unique_lock<std::mutex> & lock) {
    m_assetNotifier.wait(lock, [=]() { return !m_assetPool.isUsed(handle) || m_assetPool[handle].status != AssetStatus_Loading; });

    if (!m_assetPool.isUsed(handle)) {
      return false;
    }

    m_ptrToHandle.erase(m_assetPool[handle].pInstance.get());
    m_assetPool[handle].pInstance = nullptr;
    m_assetPool[handle].status    = AssetStatus_Unloaded;

    ++(*m_assetPool[handle].version);

    Vector<AssetHandle> invalid;
    for (AssetHandle const & dependent : m_assetPool[handle].dependent) {
      if (!reload(dependent, lock))
        invalid.pushBack(dependent);
    }

    for (AssetHandle const & dependent : invalid) {
      m_assetPool[handle].dependent.erase(dependent);
    }

    return true;
  }
} // namespace engine
