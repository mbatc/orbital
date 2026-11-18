#include "TextureLoader.h"
#include "render/GraphicsDevice.h"
#include "AssetManager.h"
#include "Rendering/Rendering.h"

using namespace bfc;

namespace engine {
  Ref<media::Surface> SurfaceLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    Ref<Stream> pStream = fs::open(uri, FileMode_ReadBinary);
    if (pStream == nullptr) {
      return nullptr;
    }

    media::Surface surface;
    if (!media::loadSurface(pStream.get(), &surface)) {
      surface.free();
      return nullptr;
    }

    return Ref<media::Surface>(new media::Surface(surface), [](media::Surface* pPtr) {
      pPtr->free();
      delete pPtr;
    });
  }

  bool SurfaceLoader::handles(URI const & uri) const {
    return media::canLoadSurface(Filename::extension(uri.path()));
  }

  bfc::Ref<graphics::Texture> Texture2DLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    Ref<media::Surface> pSurface = asset_manager::load<media::Surface>(uri, nullptr, pContext);
    if (pSurface == nullptr) {
      return nullptr;
    }

    auto pLoadList = rendering::createCommandList();
    pLoadList->setDebugName("Texture2DLoader::load");
    graphics::TextureRef pTexture;
    graphics::loadTexture2D(pLoadList.get(), &pTexture, *pSurface);
    rendering::submit(std::move(pLoadList));
    return pTexture;
  }

  bool Texture2DLoader::handles(URI const & uri) const {
    return asset_manager::canLoad<media::Surface>(uri);
  }

  bfc::Ref<bfc::graphics::Texture> TextureCache::read(bfc::Stream * pStream) const {
    bfc::TextureType type;
    if (!pStream->read(&type))
      return nullptr;

    auto surface = bfc::read<bfc::media::Surface>(pStream);
    if (!surface.has_value())
      return nullptr;

    auto pLoadList = rendering::createCommandList();
    pLoadList->setDebugName("TextureCache::read");
    graphics::TextureRef pTexture;
    graphics::loadTexture(pLoadList.get(), &pTexture, type, surface.value());
    rendering::submit(std::move(pLoadList));
    surface->free();
    return pTexture;
  }

  bool TextureCache::store(bfc::Ref<bfc::graphics::Texture> pAsset, bfc::Stream * pStream) const {
    bfc::graphics::TextureDownloadRef pDownload = rendering::createTextureDownload();
    {
      auto pLoadList = rendering::createCommandList();
      pLoadList->setDebugName("TextureCache::store");
      pLoadList->downloadTexture(pAsset, pDownload);
      rendering::submit(std::move(pLoadList));
    }

    if (!pDownload->wait())
      return false;

    return pStream->write(pAsset->getType()) && pStream->write(pDownload->view());
  }
} // namespace engine
