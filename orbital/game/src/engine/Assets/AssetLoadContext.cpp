#include "AssetLoadContext.h"
#include "AssetManager.h"
#include "Application.h"
#include "engine/Rendering/Rendering.h"
#include "render/GraphicsDevice.h"

using namespace bfc;

namespace engine {
  AssetLoadContext::AssetLoadContext(AssetManager * pAssetManager)
    : m_pAssetManager(pAssetManager) {}

  bool AssetLoadContext::contains(AssetHandle const & handle) const {
    return m_pAssetManager->contains(handle);
  }

  bool AssetLoadContext::contains(UUID const & uuid) const {
    return m_pAssetManager->contains(uuid);
  }

  AssetHandle AssetLoadContext::add(URI const & uri, type_index const & type) {
    return m_pAssetManager->add(uri, type);
  }

  AssetHandle AssetLoadContext::add(URI const & uri, StringView const & loaderID) {
    return m_pAssetManager->add(uri, loaderID);
  }

  Ref<void> AssetLoadContext::load(AssetHandle const & handle, std::optional<type_index> const & type, bool isDependency) {
    if (handle == InvalidAssetHandle) {
      return nullptr;
    }

    m_dependencies.pushBack(handle);
    Ref<void> pAsset = m_pAssetManager->load(handle, type);
    return pAsset;
  }

  Span<const AssetHandle> AssetLoadContext::getDependencies() const {
    return m_dependencies;
  }

  VirtualFileSystem * AssetLoadContext::getFileSystem() const {
    return m_pAssetManager->getFileSystem();
  }

  GraphicsDevice * AssetLoadContext::getGraphicsDevice() const {
    auto pRendering = m_pAssetManager->getApp()->findSubsystem<engine::Rendering>();
    if (pRendering == nullptr) {
      return nullptr;
    }

    return pRendering->getDevice();
  }
}
