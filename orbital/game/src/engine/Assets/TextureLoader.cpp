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
    bfc::GraphicsDevice *pDevice = pContext->getGraphicsDevice();
    Ref<media::Surface> pSurface = pContext->load<media::Surface>(uri);
    if (pSurface == nullptr) {
      return nullptr;
    }

    auto                 pLoadList = pDevice->createCommandList();
    graphics::TextureRef pTexture;
    graphics::loadTexture2D(pLoadList.get(), &pTexture, *pSurface);
    pDevice->submit(std::move(pLoadList));
    return pTexture;
  }

  bool Texture2DLoader::handles(URI const & uri, AssetManager const * pManager) const {
    return pManager->canLoad<media::Surface>(uri);
  }
} // namespace engine
