#include "AssetLoader.h"
#include "core/Map.h"
#include "core/Serialize.h"
#include "geometry/Rectangle.h"
#include "media/Pixel.h"

namespace bfc {
  namespace media {
    class Surface;
  }
  namespace graphics {
    class Texture;
  }
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
    static bfc::geometry::Rectangled getCombinedCubeMapFaceRegion(bfc::CubeMapFace face);

    struct Image {
      bfc::URI uri;
      bfc::geometry::Rectangled region = bfc::geometry::Rectangled::one();
    };

    SkyboxFormat                      format = SkyboxFormat_Unknown;
    Image                             eqrectSource;
    bfc::Map<bfc::CubeMapFace, Image> cubeSource;
    bfc::PixelFormat                  pixelFormat = bfc::PixelFormat_RGBu8;
    bfc::Vec2i                        resolution  = bfc::Vec2i(2048);
  };

  class SkyboxLoader : public AssetLoader<bfc::graphics::Texture> {
  public:
    SkyboxLoader(bfc::GraphicsDevice * pGraphicsDevice);

    virtual bfc::Ref<bfc::graphics::Texture> load(bfc::URI const & uri, AssetLoadContext * pContext) const override;
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
    template<typename Context>
    static SerializedObject write(engine::SkyboxDefinition const & o, Context const &) {
      SerializedObject src;
      switch (o.format) {
      case engine::SkyboxFormat_CubeMap: src = serialize(o.cubeSource); break;
      case engine::SkyboxFormat_Equirectangular: src = serialize(o.eqrectSource); break;
      }

      return SerializedObject::MakeMap({
        {"format", serialize(o.format)},
        {"source", src},
        {"resolution", serialize(o.resolution)},
        {"pixelFormat", serialize(o.pixelFormat)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, engine::SkyboxDefinition & o, Context const &) {
      s.get("format").readOrConstruct(o.format, engine::SkyboxFormat_Unknown);

      switch (o.format) {
      case engine::SkyboxFormat_CubeMap:
        if (s.get("source").isText()) {
          bfc::Uninitialized<bfc::URI> uri;
          s.get("source").readOrConstruct(uri.get());
          mem::construct(&o.cubeSource);
          for (size_t i = 0; i < CubeMapFace_Count; ++i) {
            CubeMapFace f = (CubeMapFace)i;
            engine::SkyboxDefinition::Image       mapping;
            mapping.uri = uri.get();
            mapping.region = engine::SkyboxDefinition::getCombinedCubeMapFaceRegion(f);
            o.cubeSource.add(f, mapping);
          }
        } else {
          s.get("source").readOrConstruct(o.cubeSource);
        }
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
  };

  template<>
  struct Serializer<engine::SkyboxDefinition::Image> {
    template<typename Context>
    static SerializedObject write(engine::SkyboxDefinition::Image const & o, Context const &) {
      return SerializedObject::MakeMap({
        { "uri", serialize(o.uri) },
        { "region", serialize(o.region) }
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, engine::SkyboxDefinition::Image & o, Context const &) {
      if (s.isText()) {
        s.readOrConstruct(o.uri);
        mem::construct(&o.region, Vec2d(0), Vec2d(1));
      } else {
        s.get("uri").readOrConstruct(o.uri);
        s.get("region").readOrConstruct(o.region, Vec2d(0), Vec2d(1));
      }
      return true;
    }
  };
} // namespace bfc
