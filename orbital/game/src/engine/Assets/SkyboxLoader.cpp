#include "SkyboxLoader.h"
#include "AssetLoadContext.h"
#include "AssetLoader.h"
#include "AssetManager.h"

#include "VirtualFileSystem.h"
#include "render/GraphicsDevice.h"
#include "render/Texture.h"

using namespace bfc;

namespace engine {
  SkyboxLoader::SkyboxLoader(GraphicsDevice * pGraphicsDevice)
    : m_pGraphicsDevice(pGraphicsDevice) {}

  Ref<Texture> SkyboxLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    std::optional<SkyboxDefinition> def = pContext->getFileSystem()->deserialize<SkyboxDefinition>(uri);
    if (!def.has_value()) {
      return nullptr;
    }

    Texture        tex;
    media::Surface sfc;
    sfc.format  = def->pixelFormat;
    sfc.size    = {def->resolution, CubeMapFace_Count};
    sfc.pitch   = 0;
    sfc.pBuffer = media::allocateSurface(sfc);

    URI baseUri = uri.withPath(Filename::parent(uri.pathView()));

    switch (def->format) {
    case SkyboxFormat_CubeMap:
      for (auto & [face, faceUri] : def->cubeSource) {
        if (face < 0 || face >= CubeMapFace_Count) {
          continue;
        }

        auto pSurface = pContext->load<media::Surface>(baseUri.resolveRelativeReference(faceUri));
        if (pSurface == nullptr) {
          continue;
        }

        media::Surface dst = sfc.slice(face);
        media::convertSurface(&dst, *pSurface);
      }
      break;
    case SkyboxFormat_Equirectangular: BFC_FAIL("Not implemented"); return nullptr;
    }

    tex.loadCubeMap(m_pGraphicsDevice, sfc);
    sfc.free();

    return NewRef<Texture>(tex);
  }

  bool SkyboxLoader::handles(URI const & uri, AssetManager const * pManager) const {
    BFC_UNUSED(pManager);

    return Filename::extension(uri.pathView()).equals("skybox", true);
  }
} // namespace engine

namespace bfc {
  SerializedObject Serializer<engine::SkyboxDefinition>::write(engine::SkyboxDefinition const & o) {
    SerializedObject src;
    switch (o.format) {
    case engine::SkyboxFormat_CubeMap: src = serialize(o.cubeSource); break;
    case engine::SkyboxFormat_Equirectangular: src = serialize(o.eqrectSource);
    }

    return SerializedObject::MakeMap({
      {"format", serialize(o.format)},
      {"source", src},
      {"resolution", serialize(o.resolution)},
      {"pixelFormat", serialize(o.pixelFormat)},
    });
  }

  bool Serializer<engine::SkyboxDefinition>::read(SerializedObject const & s, engine::SkyboxDefinition & o) {
    s.get("format").readOrConstruct(o.format, engine::SkyboxFormat_Unknown);

    switch (o.format) {
    case engine::SkyboxFormat_CubeMap:
      s.get("source").readOrConstruct(o.cubeSource);
      mem::construct(&o.eqrectSource);
      break;
    case engine::SkyboxFormat_Equirectangular:
      s.get("source").readOrConstruct(o.eqrectSource);
      mem::construct(&o.cubeSource);
      break;
    }

    s.get("resolution").readOrConstruct(o.resolution, Vec2i(2048));
    s.get("pixelFormat").readOrConstruct(o.pixelFormat, PixelFormat_RGBu8);

    return true;
  }
} // namespace bfc
