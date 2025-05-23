#include "AssetManager.h"
#include "AssetLoadContext.h"
#include "AssetLoader.h"
#include "Assets/MeshLoader.h"
#include "Assets/ShaderLoader.h"
#include "Assets/TextureLoader.h"
#include "Assets/SkyboxLoader.h"

#include "Application.h"
#include "Rendering/Rendering.h"

#include "util/Log.h"

using namespace bfc;

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

    return true;
  }

  bool AssetManager::registerLoader(StringView const & name, Ref<IAssetLoader> const & pLoader) {
    std::scoped_lock guard{m_loaderLock};
    if (findLoader_unlocked(name) != nullptr) {
      return false;
    }

    m_loaders.pushBack({name, pLoader});

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
    uint64_t version = 0;

    Ref<IAssetLoader> pLoader = nullptr;
    std::unique_lock  assetGuard{m_assetLock};
    {
      if (!m_assetPool.isUsed(handle)) {
        return nullptr;
      }

      Asset & stored = m_assetPool[handle];

      {
        std::scoped_lock guard{m_loaderLock};
        pLoader = findLoader_unlocked(stored.loader);
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

      AssetLoadContext context = { this };

      pInstance = pLoader->loadUnknown(assetUri, &context);

      assetGuard.lock();
      Asset & stored           = m_assetPool[handle];
      stored.lastVersionLoaded = version;
      stored.pInstance         = pInstance;
      stored.status            = pInstance == nullptr ? AssetStatus_Failed : AssetStatus_Loaded;
      for (AssetHandle const & dependency : context.getDependencies()) {
        m_assetPool[dependency].dependent.add(handle);
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
        *pLoadedVersion = version;

      for (auto [pVersion, ppInstance] : stored.waiting) {
        *pVersion   = version;
        *ppInstance = pInstance;
      }

      assetGuard.unlock();

      m_assetNotifier.notify_all();
    } else if (wait) {
      // Another thread started loading this asset.
      // Wait for loading to finish.
      m_assetPool[handle].waiting.pushBack(Pair{&version, &pInstance});

      m_assetNotifier.wait(assetGuard, [=, &pInstance, &version]() {
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
