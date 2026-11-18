#include "SkyboxLoader.h"
#include "AssetManager.h"

#include "AssetLoader.h"
#include "VirtualFileSystem.h"
#include "Rendering/Rendering.h"

using namespace bfc;

namespace engine {
  Ref<graphics::Texture> SkyboxLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    std::optional<SkyboxDefinition> def = fs::deserialize<SkyboxDefinition>(uri);
    if (!def.has_value()) {
      return nullptr;
    }

    media::Surface sfc;
    sfc.format  = def->pixelFormat;
    sfc.size    = {def->resolution, CubeMapFace_Count};
    sfc.pitch   = 0;
    sfc.pBuffer = media::allocateSurface(sfc);

    URI baseUri = uri.withPath(Filename::parent(uri.pathView()));

    switch (def->format) {
    case SkyboxFormat_CubeMap: {
      Vector<std::future<void>> jobs;
      for (auto & [face, faceUri] : def->cubeSource) {
        if (face < 0 || face >= CubeMapFace_Count) {
          continue;
        }
        
         jobs.pushBack(std::move(bfc::async([=]() {
          auto pSurface =
            asset_manager::load<media::Surface>(baseUri.resolveRelativeReference(faceUri.uri), nullptr, pContext);
          if (pSurface == nullptr)
            return;
          media::Surface dst = sfc.slice(face);
          media::convertSurface(&dst, pSurface->sub(faceUri.region.min, faceUri.region.max));
         })));
      }

      for (auto & job : jobs) {
        job.wait();
      }
      break;
    }
    case SkyboxFormat_Equirectangular: BFC_FAIL("Not implemented"); return nullptr;
    }

    auto pLoadList = rendering::createCommandList();
    pLoadList->setDebugName("SkyboxLoader::load");
    graphics::TextureRef tex;
    graphics::loadTextureCubeMap(pLoadList.get(), &tex, sfc);
    rendering::submit(std::move(pLoadList));

    sfc.free();
    return tex;
  }

  bool SkyboxLoader::handles(URI const & uri) const {
    return Filename::extension(uri.pathView()).equals("skybox", true);
  }

  geometry::Rectangled SkyboxDefinition::getCombinedCubeMapFaceRegion(CubeMapFace face) {
    Vec2d const dims = {4, 3};
    Vec2d const sz = Vec2d(1) / dims;

    Vec2d cell = {0, 0};
    switch (face)
    {
    case CubeMapFace_Top: cell = {1, 0}; break;
    case CubeMapFace_Left: cell = {0, 1}; break;
    case CubeMapFace_Front: cell = {1, 1}; break;
    case CubeMapFace_Right: cell = {2, 1}; break;
    case CubeMapFace_Back: cell = {3, 1}; break;
    case CubeMapFace_Bottom: cell = {1, 2}; break;
    }
    Vec2d const min = cell * sz;
    return bfc::geometry::Rectangled(min, min + sz);
  }
} // namespace engine
