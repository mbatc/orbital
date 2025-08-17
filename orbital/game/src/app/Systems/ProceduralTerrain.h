#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"

namespace bfc {
  class Mesh;
  namespace graphics {
    class Program;
  }
}

struct PerlinNoiseParams {
  uint32_t seed        = 0;
  uint32_t frequency   = 1;
  uint32_t octaves     = 6;
  float    persistence = 0.5f;
  float    lacunarity  = 2.0f;
};

struct ProceduralTerrain {
  PerlinNoiseParams heightMap;
};

class ProceduralTerrainSystem 
  : public engine::ILevelUpdate 
  , public engine::ILevelRenderDataCollector {
public:
  ProceduralTerrainSystem(bfc::Ref<engine::AssetManager> pManager);

  virtual void update(engine::Level * pLevel, bfc::Timestamp dt) override;
  virtual void collectRenderData(engine::RenderView * pRenderView, engine::Level const * pLevel) override;

private:
  engine::Asset<bfc::Mesh> m_quad;
  engine::Asset<bfc::graphics::Program> m_shader;
};

namespace bfc {
  template<>
  struct Serializer<PerlinNoiseParams> {
    template<typename Context>
    static SerializedObject write(PerlinNoiseParams const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"seed", serialize(o.seed)},
        {"frequency", serialize(o.frequency)},
        {"octaves", serialize(o.octaves)},
        {"persistence", serialize(o.persistence)},
        {"lacunarity", serialize(o.lacunarity)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, PerlinNoiseParams & o, Context const &) {
      mem::construct(&o);

      s.get("seed").read(o.seed);
      s.get("frequency").read(o.frequency);
      s.get("octaves").read(o.octaves);
      s.get("persistence").read(o.persistence);
      s.get("lacunarity").read(o.lacunarity);

      return true;
    }
  };

  template<>
  struct Serializer<ProceduralTerrain> {
    template<typename Context>
    static SerializedObject write(ProceduralTerrain const & o, Context const &) {
      return SerializedObject::MakeMap({
        { "heightMap", serialize(o.heightMap) },
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, ProceduralTerrain & o, Context const &) {
      mem::construct(&o);

      s.get("heightMap").read(o.heightMap);

      return true;
    }
  };
}
