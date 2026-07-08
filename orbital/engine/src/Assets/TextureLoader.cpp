#include "TextureLoader.h"
#include "render/GraphicsDevice.h"
#include "AssetManager.h"
#include "AssetLoadContext.h"

using namespace bfc;

namespace engine {
  Ref<media::Surface> SurfaceLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    Ref<Stream> pStream = pContext->getFileSystem()->open(uri, FileMode_ReadBinary);
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

  bool SurfaceLoader::handles(URI const & uri, AssetManager const * pManager) const {
    return media::canLoadSurface(Filename::extension(uri.path()));
  }

  Texture2DLoader::Texture2DLoader(GraphicsDevice * pGraphicsDevice)
    : m_pGraphicsDevice(pGraphicsDevice) {}

  bfc::Ref<graphics::Texture> Texture2DLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    Ref<media::Surface> pSurface = pContext->load<media::Surface>(uri);
    if (pSurface == nullptr) {
      return nullptr;
    }

    auto pLoadList = m_pGraphicsDevice->createCommandList();
    pLoadList->setDebugName("Texture2DLoader::load");
    graphics::TextureRef pTexture;
    graphics::loadTexture2D(pLoadList.get(), &pTexture, *pSurface);
    m_pGraphicsDevice->submit(std::move(pLoadList));
    return pTexture;
  }

  bool Texture2DLoader::handles(URI const & uri, AssetManager const * pManager) const {
    return pManager->canLoad<media::Surface>(uri);
  }

  TextureCache::TextureCache(bfc::GraphicsDevice * pDevice)
    : m_pGraphicsDevice(pDevice) {}

  bfc::Ref<bfc::graphics::Texture> TextureCache::read(bfc::Stream * pStream) const {
    bfc::TextureType type;
    if (!pStream->read(&type))
      return nullptr;

    auto surface = bfc::read<bfc::media::Surface>(pStream);
    if (!surface.has_value())
      return nullptr;

    auto pLoadList = m_pGraphicsDevice->createCommandList();
    pLoadList->setDebugName("TextureCache::read");
    graphics::TextureRef pTexture;
    graphics::loadTexture(pLoadList.get(), &pTexture, type, surface.value());
    m_pGraphicsDevice->submit(std::move(pLoadList));
    surface->free();
    return pTexture;
  }

  bool TextureCache::store(bfc::Ref<bfc::graphics::Texture> pAsset, bfc::Stream * pStream) const {
    bfc::graphics::TextureDownloadRef pDownload = m_pGraphicsDevice->createTextureDownload();
    {
      auto pLoadList = m_pGraphicsDevice->createCommandList();
      pLoadList->setDebugName("TextureCache::store");
      pLoadList->downloadTexture(pAsset, pDownload);
      m_pGraphicsDevice->submit(std::move(pLoadList));
    }

    if (!pDownload->wait())
      return false;

    return pStream->write(pAsset->getType()) && pStream->write(pDownload->view());
  }
} // namespace engine
