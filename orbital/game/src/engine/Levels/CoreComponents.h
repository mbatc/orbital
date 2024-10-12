#pragma once

#include "Level.h"
#include "core/Vector.h"
#include "geometry/Geometry.h"
#include "math/MathTypes.h"
#include "util/UUID.h"

namespace bfc {
  class Mesh;
  class Texture;
  class Material;
}

namespace components {
  struct Name {
    bfc::String name;
  };

  class Transform {
  public:
    bfc::Vec3d translation() const;
    bfc::Quatd orientation() const;
    bfc::Vec3d ypr() const;
    bfc::Vec3d scale() const;
    bfc::Mat4d transform() const;

    bfc::Vec3d globalTranslation(engine::Level const * pLevel) const;
    bfc::Quatd globalOrientation(engine::Level const * pLevel) const;
    bfc::Vec3d globalYpr(engine::Level const * pLevel) const;
    bfc::Vec3d globalScale(engine::Level const * pLevel) const;
    bfc::Mat4d globalTransform(engine::Level const * pLevel) const;

    bfc::Vec3d parentTranslation(engine::Level const * pLevel) const;
    bfc::Quatd parentOrientation(engine::Level const * pLevel) const;
    bfc::Vec3d parentYpr(engine::Level const * pLevel) const;
    bfc::Vec3d parentScale(engine::Level const * pLevel) const;
    bfc::Mat4d parentTransform(engine::Level const * pLevel) const;

    engine::EntityID parent() const;

    void setTranslation(bfc::Vec3d const & translation);
    void setOrientation(bfc::Quatd const & orientation);
    void setYpr(bfc::Vec3d const & ypr);
    void setScale(bfc::Vec3d const & scale);
    void setTransform(bfc::Mat4d const & transform);

    void setGlobalTranslation(engine::Level const * pLevel, bfc::Vec3d const & translation);
    void setGlobalOrientation(engine::Level const * pLevel, bfc::Quatd const & orientation);
    void setGlobalYpr(engine::Level const * pLevel, bfc::Vec3d const & ypr);
    void setGlobalScale(engine::Level const * pLevel, bfc::Vec3d const & scale);
    void setGlobalTransform(engine::Level const * pLevel, bfc::Mat4d const & transform);

    void setParent(engine::Level const * pLevel, engine::EntityID const & entityID) const;

  private:
    engine::EntityID              m_parent;
    bfc::Vector<engine::EntityID> m_children;

    bfc::Vec3d m_translation = {0, 0, 0};
    bfc::Quatd m_orientation = glm::identity<bfc::Quatd>();
    bfc::Vec3d m_scale       = {1, 1, 1};
  };

  enum LightType {
    LightType_Sun,
    LightType_Point,
    LightType_Spot,
    LightType_Count,
  };

  struct Light {
    LightType type = LightType_Sun;

    bfc::Vec3 colour      = bfc::Vec3(0);
    bfc::Vec3 ambient     = bfc::Vec3(1);
    bfc::Vec3 attenuation = {0, 0, 1};

    float strength       = 1.0f;
    float innerConeAngle = glm::radians(30.0f);
    float outerConeAngle = glm::radians(35.0f);

    bool castShadows = true;
  };

  struct Skybox {
    bfc::Ref<bfc::Texture> pTexture;
  };

  struct StaticMesh {
    bfc::Ref<bfc::Mesh> pMesh;
    bfc::Vector<bfc::Ref<bfc::Material>> materials;
    bool castShadows = true;
  };

  /// Defines a area in the scene which post processing effects are applied to.
  struct PostProcessVolume {
    bfc::geometry::Boxd extents = {bfc::Vec3d(0), 10.0};

    bool infinite = true;
    bool enabled  = true;

    engine::EntityID effects; ///< The entity that contains the effects to apply inside this volume.
  };

  struct PostProcess_Tonemap {
    float exposure = 1.0f;
  };

  struct PostProcess_Bloom {
    float strength      = 0.04f;
    float filterRadius  = 0.005f;
    float threshold     = 0.25f;
    float dirtIntensity = 1.0f;

    bfc::Ref<bfc::Texture> dirt;
  };

  struct PostProcess_SSAO {
    float strength = 1.0f;
    float radius   = 0.50f;
    float bias     = 0.1f;
  };

  struct PostProcess_SSR {
    float maxDistance = 15.0f;
    float resolution  = 0.30f;
    int   steps       = 10;
    float thickness   = 0.5f;
  };
} // namespace components

namespace bfc {
  template<>
  struct Serializer<components::Name> {
    inline static SerializedObject write(components::Name const & o) {
      return serialize(o.name);
    }

    inline static bool read(SerializedObject const & s, components::Name & o) {
      return s.read(o.name);
    }
  };

  template<>
  struct Serializer<components::Transform> {
    inline static SerializedObject write(components::Transform const & o) {
      return SerializedObject::MakeMap({
        { "translation", serialize(o.translation()) },
        { "ypr", serialize(glm::degrees(o.ypr())) },
        { "scale", serialize(o.scale()) },
      });
    }

    inline static bool read(SerializedObject const & s, components::Transform & o) {
      Vec3d translation, ypr, scale;
      deserialize(s.get("translation"), translation);
      deserialize(s.get("ypr"), ypr);
      deserialize(s.get("scale"), scale);

      mem::construct(&o);
      o.setTranslation(translation);
      o.setYpr(glm::radians(ypr));
      o.setScale(scale);

      return true;
    }
  };

  // template<>
  // struct Serializer<components::StaticMesh> {
  //   inline static SerializedObject write(components::StaticMesh const & o) {
  //     return serialize(o.);
  //   }
  // 
  //   inline static bool read(SerializedObject const & s, components::StaticMesh & o) {
  //     return deserialize(s, o.name);
  //   }
  // };

  template<>
  struct EnumValueMap<components::LightType> {
    inline static Map<components::LightType, String> const mapping = {{components::LightType_Sun,   "sun"},
                                                                      {components::LightType_Point, "point"},
                                                                      {components::LightType_Spot,  "spot"}};
  };

  template<>
  struct Serializer<components::Light> {
    inline static SerializedObject write(components::Light const & o) {
      return SerializedObject::MakeMap({
        {"type", serialize(o.type)},
        {"colour", serialize(o.colour)},
        {"ambient", serialize(o.ambient)},
        {"attenuation", serialize(o.type)},
        {"strength", serialize(o.strength)},
        {"innerConeAngle", serialize(o.innerConeAngle)},
        {"outerConeAngle", serialize(o.outerConeAngle)},
        {"castShadows", serialize(o.castShadows)},
      });
    }

    inline static bool read(SerializedObject const & s, components::Light & o) {
      mem::construct(&o);

      s.get("type").read(o.type);
      s.get("colour").read(o.colour);
      s.get("ambient").read(o.ambient);
      s.get("attenuation").read(o.attenuation);
      s.get("strength").read(o.strength);
      s.get("innerConeAngle").read(o.innerConeAngle);
      s.get("outerConeAngle").read(o.outerConeAngle);
      s.get("castShadows").read(o.castShadows);

      return true;
    }
  };

  // template<>
  // struct Serializer<components::Skybox> {
  //   inline static SerializedObject write(components::Skybox const & o) {
  //     return serialize(o.name);
  //   }
  // 
  //   inline static bool read(SerializedObject const & s, components::Skybox & o) {
  //     return deserialize(s, o.name);
  //   }
  // };

  template<>
  struct Serializer<components::PostProcessVolume> {
    inline static SerializedObject write(components::PostProcessVolume const & o) {
      return SerializedObject::MakeMap({
        {"enabled",  serialize(o.enabled)},
        {"extents",  serialize(o.extents)},
        {"infinite", serialize(o.infinite)},
        {"effects",  serialize(o.effects)},
      });
    }
  
    inline static bool read(SerializedObject const & s, components::PostProcessVolume & o) {
      mem::construct(&o);

      s.get("enabled").read(o.enabled);
      s.get("extents").read(o.extents);
      s.get("infinite").read(o.infinite);
      s.get("effects").read(o.effects);

      return true;
    }
  };

  template<>
  struct Serializer<components::PostProcess_Tonemap> {
    inline static SerializedObject write(components::PostProcess_Tonemap const & o) {
      return SerializedObject::MakeMap({
        {"exposure", serialize(o.exposure)},
      });
    }

    inline static bool read(SerializedObject const & s, components::PostProcess_Tonemap & o) {
      mem::construct(&o);

      s.get("exposure").read(o.exposure);

      return true;
    }
  };

  template<>
  struct Serializer<components::PostProcess_Bloom> {
    inline static SerializedObject write(components::PostProcess_Bloom const & o) {
      return SerializedObject::MakeMap({
        {"filterRadius", serialize(o.filterRadius)},
        {"strength", serialize(o.strength)},
        {"threshold", serialize(o.threshold)},
        {"dirtIntensity", serialize(o.dirtIntensity)},
        // {"dirt", serialize(o.dirt)},
      });
    }

    inline static bool read(SerializedObject const & s, components::PostProcess_Bloom & o) {
      mem::construct(&o);

      s.get("filterRadius").read(o.filterRadius);
      s.get("strength").read(o.strength);
      s.get("threshold").read(o.threshold);
      s.get("dirtIntensity").read(o.dirtIntensity);
      // s.get("dirt").read(o.dirt);

      return true;
    }
  };

  template<>
  struct Serializer<components::PostProcess_SSAO> {
    inline static SerializedObject write(components::PostProcess_SSAO const & o) {
      return SerializedObject::MakeMap({
        {"bias", serialize(o.bias)},
        {"radius", serialize(o.radius)},
        {"strength", serialize(o.strength)},
      });
    }

    inline static bool read(SerializedObject const & s, components::PostProcess_SSAO & o) {
      mem::construct(&o);

      s.get("bias").read(o.bias);
      s.get("radius").read(o.radius);
      s.get("strength").read(o.strength);

      return true;
    }
  };

  template<>
  struct Serializer<components::PostProcess_SSR> {
    inline static SerializedObject write(components::PostProcess_SSR const & o) {
      return SerializedObject::MakeMap({
        {"maxDistance", serialize(o.maxDistance)},
        {"resolution", serialize(o.resolution)},
        {"steps", serialize(o.steps)},
        {"thickness", serialize(o.thickness)},
      });
    }

    inline static bool read(SerializedObject const & s, components::PostProcess_SSR & o) {
      mem::construct(&o);

      s.get("maxDistance").read(o.maxDistance);
      s.get("resolution").read(o.resolution);
      s.get("steps").read(o.steps);
      s.get("thickness").read(o.thickness);

      return true;
    }
  };
}
