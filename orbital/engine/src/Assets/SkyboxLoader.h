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
    virtual bfc::Ref<bfc::graphics::Texture> load(bfc::URI const & uri, AssetLoadContext * pContext) const override;
    virtual bool                   handles(bfc::URI const & uri) const override;
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
  struct Serialization<engine::SkyboxDefinition> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, engine::SkyboxDefinition const & o) {
      SerializedObject src;
      switch (o.format) {
      case engine::SkyboxFormat_CubeMap: src = ar(o.cubeSource); break;
      case engine::SkyboxFormat_Equirectangular: src = ar(o.eqrectSource); break;
      }

      return SerializedObject::MakeMap({
        {"format", ar(o.format)},
        {"source", src},
        {"resolution", ar(o.resolution)},
        {"pixelFormat", ar(o.pixelFormat)},
      });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, engine::SkyboxDefinition & o) {
      ar(s.get("format"), bfc::ReadOrConstruct(o.format, engine::SkyboxFormat_Unknown));

      switch (o.format) {
      case engine::SkyboxFormat_CubeMap:
        if (s.get("source").isText()) {
          bfc::Uninitialized<bfc::URI> uri;
          ar(s.get("source"), bfc::ReadOrConstruct(uri.get()));
          mem::construct(&o.cubeSource);
          for (size_t i = 0; i < CubeMapFace_Count; ++i) {
            CubeMapFace f = (CubeMapFace)i;
            engine::SkyboxDefinition::Image       mapping;
            mapping.uri = uri.get();
            mapping.region = engine::SkyboxDefinition::getCombinedCubeMapFaceRegion(f);
            o.cubeSource.add(f, mapping);
          }
        } else {
          ar(s.get("source"), bfc::ReadOrConstruct(o.cubeSource));
        }
        mem::construct(&o.eqrectSource);
        break;
      case engine::SkyboxFormat_Equirectangular:
        ar(s.get("source"), bfc::ReadOrConstruct(o.eqrectSource));
        mem::construct(&o.cubeSource);
        break;
      }

      ar(s.get("resolution"), bfc::ReadOrConstruct(o.resolution, Vec2i(2048)));
      ar(s.get("pixelFormat"), bfc::ReadOrConstruct(o.pixelFormat, PixelFormat_RGBu8));

      return true;
    }
  };

  template<>
  struct Serialization<engine::SkyboxDefinition::Image> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, engine::SkyboxDefinition::Image const & o) {
      return SerializedObject::MakeMap({
        { "uri", ar(o.uri) },
        { "region", ar(o.region) }
      });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, engine::SkyboxDefinition::Image & o) {
      if (s.isText()) {
        ar(s, bfc::ReadOrConstruct(o.uri));
        mem::construct(&o.region, Vec2d(0), Vec2d(1));
      } else {
        ar(s.get("uri"), bfc::ReadOrConstruct(o.uri));
        ar(s.get("region"), bfc::ReadOrConstruct(o.region, Vec2d(0), Vec2d(1)));
      }
      return true;
    }
  };
} // namespace bfc
