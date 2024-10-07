#include "TextureLoader.h"
#include "render/Texture.h"
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

  Ref<Texture> Texture2DLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    Ref<media::Surface> pSurface = pContext->load<media::Surface>(uri);
    if (pSurface == nullptr) {
      return nullptr;
    }

    Ref<Texture> pTexture = NewRef<Texture>();

    pTexture->load2D(m_pGraphicsDevice, *pSurface);
    return pTexture;
  }

  bool Texture2DLoader::handles(URI const & uri, AssetManager const * pManager) const {
    return pManager->canLoad<media::Surface>(uri);
  }
} // namespace engine
