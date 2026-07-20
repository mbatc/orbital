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

    double radius = 1;

    float    frequency   = 1.0f;
    uint32_t octaves     = 6;
    float    persistance = 0.5f;
    float    lacurnarity = 2.0f;
  };

  struct PlanetAtmosphere {
    bfc::Vec3 sunDirection     = bfc::math::normalize(bfc::Vec3(1, 1, 1));
    float     sunIntensity     = 20;
    float     innerRadius      = 1;
    float     outerRadius      = 1.025f;
    float     rayleighConstant = 0.0025f;
    float     mieConstant      = 0.0010f;
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
        {"radius", serialize(o.radius)},
        {"frequency", serialize(o.frequency)},
        {"octaves", serialize(o.octaves)},
        {"persistance", serialize(o.persistance)},
        {"lacurnarity", serialize(o.lacurnarity)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::ProceduralPlanet & o, Context const &) {
      s.get("seed").readOrConstruct(o.seed, 0);
      s.get("scale").readOrConstruct(o.scale, 1.0f);
      s.get("minHeight").readOrConstruct(o.minHeight, 0.0f);
      s.get("maxHeight").readOrConstruct(o.maxHeight, 1.0f);
      s.get("radius").readOrConstruct(o.radius, 1.0);
      s.get("frequency").readOrConstruct(o.frequency, 1.0f);
      s.get("octaves").readOrConstruct(o.octaves, 6);
      s.get("persistance").readOrConstruct(o.persistance, 0.5f);
      s.get("lacurnarity").readOrConstruct(o.lacurnarity, 2.0f);
      return true;
    }
  };

  template<>
  struct Serializer<components::PlanetAtmosphere> {
    template<typename Context>
    static SerializedObject write(components::PlanetAtmosphere const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"innerRadius", serialize(o.innerRadius)},
        {"outerRadius", serialize(o.outerRadius)},
        {"sunDirection", serialize(o.sunDirection)},
        {"sunIntensity", serialize(o.sunIntensity)},
        {"rayleighConstant", serialize(o.rayleighConstant)},
        {"mieConstant", serialize(o.mieConstant)}
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::PlanetAtmosphere & o, Context const &) {
      s.get("innerRadius").readOrConstruct(o.innerRadius, 1.0f);
      s.get("outerRadius").readOrConstruct(o.outerRadius, 1.025f);
      s.get("sunDirection").readOrConstruct(o.sunDirection, bfc::math::normalize(bfc::Vec3(1)));
      s.get("sunIntensity").readOrConstruct(o.sunIntensity, 20.0f);
      s.get("rayleighConstant").readOrConstruct(o.rayleighConstant, 0.0025f);
      s.get("mieConstant").readOrConstruct(o.mieConstant, 0.0010f);
      return true;
    }
  };
} // namespace bfc
