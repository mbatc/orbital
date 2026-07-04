#pragma once

#include "core/typeindex.h"
#include "core/URI.h"
#include "util/UUID.h"
#include "util/ThreadPool.h"

#include <optional>

namespace bfc {
  namespace graphics {
    class CommandList;
  }
  class GraphicsDevice;
}

namespace engine {
  struct AssetHandle;
  class AssetManager;
  class VirtualFileSystem;

  class AssetLoadContext {
  public:
    AssetLoadContext(AssetManager * pAssetManager);

    /// Check if an asset is contained in the asset manager by its handle.
    bool contains(AssetHandle const & handle) const;

    /// Check if an asset is contained in the asset manager by its ID.
    bool contains(bfc::UUID const & uuid) const;

    /// Find the handle of an asset by its uuid.
    AssetHandle find(bfc::UUID const & uuid) const;

    /// Add an asset to the asset manager.
    template<typename T>
    AssetHandle add(bfc::URI const & uri) {
      return add(uri, bfc::TypeID<T>());
    }

    /// Add an asset to the asset manager by its type.
    AssetHandle add(bfc::URI const & uri, bfc::type_index const & type);

    /// Add an asset to the asset manager, specifying a loader type.
    AssetHandle add(bfc::URI const & uri, bfc::StringView const & loaderID);

    /// Get the asset associated with `handle`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(AssetHandle const & handle, bool isDependency = true) {
      return std::static_pointer_cast<T>(load(handle, bfc::TypeID<T>(), isDependency));
    }

    /// Get the asset associated with `uri`.
    /// Registers the asset if it is not already added to the asset manager.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(bfc::URI const & uri, bool isDependency = true) {
      return load<T>(add<T>(uri));
    }

    /// Get the asset associated with `uri`.
    /// Registers the asset if it is not already added to the asset manager.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    template<typename T>
    bfc::Ref<T> load(bfc::UUID const & uuid, bool isDependency = true) {
      return load<T>(add<T>(uuid));
    }

    /// Get the asset associated with `handle`.
    /// @returns The loaded asset.
    /// @retval nullptr `handle` is invalid or the asset type does not match `type`.
    bfc::Ref<void> load(AssetHandle const & handle, std::optional<bfc::type_index> const & type = std::nullopt, bool isDependency = true);

    template<typename T>
    std::future<bfc::Ref<T>> loadAsync(bfc::URI const & uri, bool isDependency = true) {
      return bfc::async([=]() { return load<T>(uri, isDependency); });
    }

    template<typename T>
    std::future<bfc::Ref<T>> loadAsync(bfc::UUID const & uuid, bool isDependency = true) {
      return bfc::async([=]() { return load<T>(uuid, isDependency); });
    }

    template<typename T>
    std::future<bfc::Ref<T>> loadAsync(AssetHandle const & handle, bool isDependency = true) {
      return bfc::async([=]() { return load<T>(handle, isDependency); });
    }

    std::future<bfc::Ref<void>> loadAsync(AssetHandle const &                    handle,
                                          std::optional<bfc::type_index> const & type           = std::nullopt,
                                          bool                                   isDependency = true);

    template<typename T>
    bool canLoad(bfc::URI const & uri) const {
      return m_pAssetManager->canLoad<T>(uri);
    }
    
    bfc::Span<const AssetHandle> getDependencies() const;

    VirtualFileSystem *   getFileSystem() const;
    bfc::GraphicsDevice * getGraphicsDevice() const;

  private:
    std::mutex               m_dependencyLock;
    bfc::Vector<AssetHandle> m_dependencies;
    AssetManager *           m_pAssetManager = nullptr;
  };
} // namespace engine
