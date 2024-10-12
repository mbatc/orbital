#include "AssetLoader.h"
#include "core/Map.h"
#include "core/Serialize.h"
#include "media/Pixel.h"

namespace bfc {
  namespace media {
    class Surface;
  }
  class Texture;
  class GraphicsDevice;
  enum CubeMapFace;
} // namespace bfc

namespace engine {
  enum SkyboxFormat {
    SkyboxFormat_Unknown = -1,
    SkyboxFormat_CubeMap,
    SkyboxFormat_Equirectangular,
    SkyboxFormat_Count,
  };

  class SkyboxDefinition {
  public:
    SkyboxFormat format;

    bfc::URI                             eqrectSource;
    bfc::Map<bfc::CubeMapFace, bfc::URI> cubeSource;

    bfc::PixelFormat pixelFormat = bfc::PixelFormat_RGBu8;
    bfc::Vec2i resolution  = bfc::Vec2i(2048);
  };

  class SkyboxLoader : public AssetLoader<bfc::Texture> {
  public:
    SkyboxLoader(bfc::GraphicsDevice * pGraphicsDevice);

    virtual bfc::Ref<bfc::Texture> load(bfc::URI const & uri, AssetLoadContext * pContext) const override;
    virtual bool                   handles(bfc::URI const & uri, AssetManager const * pManager) const override;

  private:
    bfc::GraphicsDevice * m_pGraphicsDevice = nullptr;
  };
} // namespace engine

namespace bfc {
  template<>
  struct EnumValueMap<engine::SkyboxFormat> {
    // inline static Vector<any> const mapping;
    inline static Map<engine::SkyboxFormat, String> const mapping = {
      {engine::SkyboxFormat_CubeMap, "cube"}, {engine::SkyboxFormat_Equirectangular, "eqrect"}, {engine::SkyboxFormat_Unknown, "unknown"}};
  };

  template<>
  struct Serializer<engine::SkyboxDefinition> {
    static SerializedObject write(engine::SkyboxDefinition const & o);
    static bool             read(SerializedObject const & s, engine::SkyboxDefinition & o);
  };
} // namespace bfc
