#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"
#include "Assets/AssetManager.h"

namespace engine {
  class Rendering;
}

namespace bfc {
  class Mesh;
  namespace graphics {
    class Program;
  }
}

namespace components {
  struct ProceduralPlanet {
    uint32_t seed   = 0;
    float scale     = 1;
    float minHeight = 0;
    float maxHeight = 1;
  };
} // namespace components

class ProceduralTerrainSystem
  : public engine::ILevelUpdate
  , public engine::ILevelPlay
  , public engine::ILevelPause
  , public engine::ILevelStop
  , public engine::ILevelRenderDataCollector {
public:
  ProceduralTerrainSystem(bfc::Ref<engine::Rendering> const & pRendering, bfc::Ref<engine::AssetManager> const & pAssets);

  virtual void update(engine::Level * pLevel, bfc::Timestamp dt) override;
  virtual void play(engine::Level * pLevel) override;
  virtual void pause(engine::Level * pLevel) override;
  virtual void stop(engine::Level * pLevel) override;
  virtual void collectRenderData(engine::RenderView * pRenderView, engine::Level const * pLevel) override;
};

namespace bfc {
  template<>
  struct Serializer<components::ProceduralPlanet> {
    template<typename Context>
    static SerializedObject write(components::ProceduralPlanet const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"seed", serialize(o.seed)},
        {"scale", serialize(o.scale)},
        {"minHeight", serialize(o.minHeight)},
        {"maxHeight", serialize(o.maxHeight)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::ProceduralPlanet & o, Context const &) {
      mem::construct(&o);

      s.get("seed").readOrConstruct(o.seed, 0);
      s.get("scale").readOrConstruct(o.scale, 1.0f);
      s.get("minHeight").readOrConstruct(o.minHeight, 0.0f);
      s.get("maxHeight").readOrConstruct(o.maxHeight, 1.0f);

      return true;
    }
  };
} // namespace bfc
