#pragma once

#include "Level.h"
#include "core/Vector.h"
#include "geometry/Geometry.h"
#include "math/MathTypes.h"
#include "util/UUID.h"
#include "render/GraphicsDevice.h"
#include "util/ThreadPool.h"

namespace bfc {
  class Mesh;
  class Material;
} // namespace bfc

namespace components {
  struct Name {
    bfc::String name;
  };

  class Transform {
  public:
    static bfc::Vec3d applyToPoint(bfc::Mat4d const & transform, bfc::Vec3d const & point);
    static bfc::Vec3d applyToDirection(bfc::Mat4d const & transform, bfc::Vec3d const & direction);

    bfc::Vec3d translation() const;
    bfc::Quatd orientation() const;
    bfc::Vec3d ypr() const;
    bfc::Vec3d scale() const;
    bfc::Mat4d transform() const;
    bfc::Mat4d transformInverse() const;

    bfc::Vec3d forward() const;
    bfc::Vec3d right() const;
    bfc::Vec3d up() const;

    bfc::Vec3d globalTranslation(engine::Level const * pLevel) const;
    bfc::Quatd globalOrientation(engine::Level const * pLevel) const;
    bfc::Vec3d globalYpr(engine::Level const * pLevel) const;
    bfc::Vec3d globalScale(engine::Level const * pLevel) const;
    bfc::Mat4d globalTransform(engine::Level const * pLevel) const;
    bfc::Mat4d globalTransformInverse(engine::Level const * pLevel) const;

    bfc::Vec3d globalForward(engine::Level const * pLevel) const;
    bfc::Vec3d globalRight(engine::Level const * pLevel) const;
    bfc::Vec3d globalUp(engine::Level const * pLevel) const;

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
    void lookAt(bfc::Vec3d const & direction, bfc::Vec3d const & up = bfc::math::up<double>);

    void translate(bfc::Vec3d const & translation);
    void rotate(bfc::Quatd const & rotation);
    void scale(bfc::Vec3d const & amount);
    void scale(double const & amount);

    void setGlobalTranslation(engine::Level const * pLevel, bfc::Vec3d const & translation);
    void setGlobalOrientation(engine::Level const * pLevel, bfc::Quatd const & orientation);
    void setGlobalYpr(engine::Level const * pLevel, bfc::Vec3d const & ypr);
    void setGlobalScale(engine::Level const * pLevel, bfc::Vec3d const & scale);
    void setGlobalLookAt(engine::Level const * pLevel, bfc::Vec3d const & direction, bfc::Vec3d const & up = bfc::math::up<double>);
    void setGlobalTransform(engine::Level const * pLevel, bfc::Mat4d const & transform);

    bool setParent(engine::Level * pLevel, engine::EntityID const & entityID);
    bool addChild(engine::Level * pLevel, engine::EntityID const & entityID);
    bool removeChild(engine::Level * pLevel, engine::EntityID const & entityID);
    bool isDescendantOf(engine::Level * pLevel, engine::EntityID const & entityID) const;

    bfc::Span<engine::EntityID> children() const;

  private:
    engine::EntityID              m_parent = engine::InvalidEntity;
    bfc::Vector<engine::EntityID> m_children;

    bfc::Vec3d m_translation = {0, 0, 0};
    bfc::Quatd m_orientation = glm::identity<bfc::Quatd>();
    bfc::Vec3d m_scale       = {1, 1, 1};
  };

  struct Camera {
    float fov       = glm::radians(60.0f);
    float nearPlane = 0.01f;
    float farPlane  = 1000.0f;

    bfc::Vec2 viewportPosition = {0, 0};
    bfc::Vec2 viewportSize     = {1, 1};

    bfc::graphics::RenderTargetRef renderTarget = bfc::InvalidGraphicsResource;

    bfc::Mat4 projectionMat(float aspect = 1) const;
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
    bfc::graphics::TextureRef pTexture;
  };

  struct ShadedMaterial {
    engine::Asset<bfc::Material>          pMaterial;
    engine::Asset<bfc::graphics::Program> pProgram;
  };

  struct StaticMesh {
    bfc::Ref<bfc::Mesh>         pMesh;
    bfc::Vector<ShadedMaterial> materials;
    bool                        castShadows    = true;
    bool                        useTesselation = false;
    bfc::Face                   faces          = bfc::Face_FrontAndBack;
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

    bfc::graphics::TextureRef dirt;
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

namespace engine {
  template<>
  struct LevelComponent_OnCopy<components::Transform> {
    inline static void onCopy(LevelCopyContext * pContext, Level * pDstLevel, EntityID dstEntity, Level const & srcLevel,
                            components::Transform const & component) {
      BFC_UNUSED(srcLevel);

      components::Transform & dst = pDstLevel->replace<components::Transform>(dstEntity);
      dst.setTranslation(component.translation());
      dst.setOrientation(component.orientation());
      dst.setScale(component.scale());

      pContext->defer([dstEntity, parent = component.parent()](LevelCopyContext * pContext, engine::Level * pDstLevel) {
        if (components::Transform * pTransform = pDstLevel->tryGet<components::Transform>(dstEntity))
          pTransform->setParent(pDstLevel, pContext->remap(parent));
      });
    }
  };

  template<>
  struct LevelComponent_OnPreErase<components::Transform> {
    inline static void onPreErase(components::Transform * pComponent, Level * pLevel) {
      pComponent->setParent(pLevel, InvalidEntity); // Remove from parent entity
    }
  };
}

namespace bfc {
  template<>
  struct Serialization<components::Transform> {
    inline static SerializedObject write(engine::LevelSerializer::Writer & ar, components::Transform const & o) {
      auto ret = SerializedObject::MakeMap({{"translation", ar(o.translation())},
                                            {"ypr", ar(glm::degrees(o.ypr()))},
                                            {"scale", ar(o.scale())},
                                            {"parent", ar(o.parent())}});
      return ret;
    }

    inline static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::Transform & o) {
      Vec3d translation, ypr, scale;
      bfc::read(s.get("translation"), translation);
      bfc::read(s.get("ypr"), ypr);
      bfc::read(s.get("scale"), scale);

      mem::construct(&o);
      o.setTranslation(translation);
      o.setYpr(glm::radians(ypr));
      o.setScale(scale);

      engine::EntityID parentID;
      ar(s.get("parent"), parentID);
      if (parentID != engine::InvalidEntity) {
        ar.deferRead([entity = ar.entity(), parentID](engine::Level & level) {
          components::Transform * pTransform = level.tryGet<components::Transform>(entity);
          if (pTransform != nullptr) {
            pTransform->setParent(&level, parentID);
          }
        });
      }

      return true;
    }
  };

  // A Specific serializer type that should be specialized for more control over entity component serialization.
  template<>
  struct Serialization<components::ShadedMaterial> {
    // Default implementation delegates to a bfc serialize implementation for the component type.
    inline static SerializedObject write(engine::LevelSerializer::Writer & ar, components::ShadedMaterial const & o) {
      return SerializedObject::MakeMap({
        {"material", ar(o.pMaterial)},
        {"shader", ar(o.pProgram)},
      });
    }

    inline static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::ShadedMaterial & o) {
      if (s.get("uri") || s.get("uuid")) {
        ar.readAsync(&components::ShadedMaterial::pMaterial, s);
      } else {
        ar.readAsync(&components::ShadedMaterial::pMaterial, s.get("material"));
        ar.readAsync(&components::ShadedMaterial::pProgram, s.get("shader"));
        ar(s.get("material"), o.pMaterial);
        ar(s.get("shader"), o.pProgram);
      }
      return true;
    }
  };

  // A Specific serializer type that should be specialized for more control over entity component serialization.
  template<>
  struct Serialization<components::StaticMesh> {
    // Default implementation delegates to a bfc serialize implementation for the component type.
    inline static SerializedObject write(engine::LevelSerializer::Writer & ar, components::StaticMesh const & o) {
      return SerializedObject::MakeMap({
        {"castShadows", ar(o.castShadows)},
        {"useTesselation", ar(o.useTesselation)},
        {"materials", ar(o.materials)},
        {"mesh", ar(o.pMesh)},
      });
    }

    inline static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::StaticMesh & o) {
      ar(s.get("castShadows"), bfc::ReadOrConstruct{o.castShadows, true});
      ar(s.get("useTesselation"), bfc::ReadOrConstruct{o.useTesselation, false });

      ar.readAsync(&components::StaticMesh::pMesh, s.get("mesh"));
      ar.readAsync(&components::StaticMesh::materials, s.get("materials"));

      return true;
    }
  };

  template<>
  struct Serialization<components::Skybox> {
    inline static SerializedObject write(engine::LevelSerializer::Writer & ar, components::Skybox const & o) {
      return SerializedObject::MakeMap({{"texture", ar(o.pTexture)}});
    }

    inline static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::Skybox & o) {
      ar.readAsync(&components::Skybox::pTexture, s.get("texture"));
      return true;
    }
  };

  template<>
  struct Serialization<components::PostProcess_Bloom> {
    inline static SerializedObject write(engine::LevelSerializer::Writer & ar, components::PostProcess_Bloom const & o) {
      return SerializedObject::MakeMap({{"filterRadius", ar(o.filterRadius) },
                                        {"strength", ar(o.strength)},
                                        {"threshold", ar(o.threshold)},
                                        {"dirtIntensity", ar(o.dirtIntensity)},
                                        {"dirt", ar(o.dirt) }});
    }

    inline static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::PostProcess_Bloom & o) {
      ar(s.get("filterRadius"), bfc::ReadOrConstruct{ o.filterRadius, 0.005f });
      ar(s.get("strength"), bfc::ReadOrConstruct{ o.strength, 0.04f });
      ar(s.get("threshold"), bfc::ReadOrConstruct{ o.threshold, 0.25f });
      ar(s.get("dirtIntensity"), bfc::ReadOrConstruct{ o.dirtIntensity, 1.0f });

      ar.readAsync(&components::PostProcess_Bloom::dirt, s.get("dirt"));

      return true;
    }
  };

  template<>
  struct Serialization<components::Name> {
    static SerializedObject write(engine::LevelSerializer::Writer & ar, components::Name const & o) {
      return serialize(o.name);
    }

    static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::Name & o) {
      ar(s, bfc::ReadOrConstruct{ o.name });
      return true;
    }
  };

  template<>
  struct Serialization<components::Camera> {
    static SerializedObject write(engine::LevelSerializer::Writer & ar, components::Camera const & o) {
      return SerializedObject::MakeMap({
        {"farPlane", ar(o.farPlane)},
        {"nearPlane", ar(o.nearPlane)},
        {"viewportSize", ar(o.viewportSize)},
        {"viewportPosition", ar(o.viewportPosition)},
        {"fov", ar(glm::degrees(o.fov))}
      });
    }

    static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::Camera & o) {
      mem::construct(&o);

      ar(s.get("farPlane"), o.farPlane);
      ar(s.get("nearPlane"), o.nearPlane);
      ar(s.get("viewportSize"), o.viewportSize);
      ar(s.get("viewportPosition"), o.viewportPosition);
      ar(s.get("fov"), o.fov);

      o.fov = glm::radians(o.fov);

      return true;
    }
  };

  template<>
  struct EnumValueMap<components::LightType> {
    inline static Map<components::LightType, String> const mapping = {
      {components::LightType_Sun, "sun"}, {components::LightType_Point, "point"}, {components::LightType_Spot, "spot"}};
  };

  template<>
  struct Serialization<components::Light> {
    static SerializedObject write(engine::LevelSerializer::Writer & ar, components::Light const & o) {
      return SerializedObject::MakeMap({
        {"type", ar(o.type)},
        {"colour", ar(o.colour)},
        {"ambient", ar(o.ambient)},
        {"attenuation", ar(o.attenuation)},
        {"strength", ar(o.strength)},
        {"innerConeAngle", ar(glm::degrees(o.innerConeAngle))},
        {"outerConeAngle", ar(glm::degrees(o.outerConeAngle))},
        {"castShadows", ar(o.castShadows)},
      });
    }

    static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::Light & o) {
      mem::construct(&o);

      ar(s.get("type"), o.type);
      ar(s.get("colour"), o.colour);
      ar(s.get("ambient"), o.ambient);
      ar(s.get("attenuation"), o.attenuation);
      ar(s.get("strength"), o.strength);
      ar(s.get("innerConeAngle"), o.innerConeAngle);
      ar(s.get("outerConeAngle"), o.outerConeAngle);
      ar(s.get("castShadows"), o.castShadows);

      o.innerConeAngle = glm::radians(o.innerConeAngle);
      o.outerConeAngle = glm::radians(o.outerConeAngle);

      return true;
    }
  };

  template<>
  struct Serialization<components::PostProcessVolume> {
    static SerializedObject write(engine::LevelSerializer::Writer & ar, components::PostProcessVolume const & o) {
      return SerializedObject::MakeMap({
        {"enabled", ar(o.enabled)},
        {"extents", ar(o.extents)},
        {"infinite", ar(o.infinite)},
        {"effects", ar(o.effects)},
      });
    }

    static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::PostProcessVolume & o) {
      mem::construct(&o);

      ar(s.get("enabled"), o.enabled);
      ar(s.get("extents"), o.extents);
      ar(s.get("infinite"), o.infinite);
      ar(s.get("effects"), o.effects);

      return true;
    }
  };

  template<>
  struct Serialization<components::PostProcess_Tonemap> {
    static SerializedObject write(engine::LevelSerializer::Writer & ar, components::PostProcess_Tonemap const & o) {
      return SerializedObject::MakeMap({
        {"exposure", serialize(o.exposure)},
      });
    }

    static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::PostProcess_Tonemap & o) {
      mem::construct(&o);

      ar(s.get("exposure"), o.exposure);

      return true;
    }
  };

  template<>
  struct Serialization<components::PostProcess_SSAO> {
    static SerializedObject write(engine::LevelSerializer::Writer & ar, components::PostProcess_SSAO const & o) {
      return SerializedObject::MakeMap({
        {"bias", ar(o.bias)},
        {"radius", ar(o.radius)},
        {"strength", ar(o.strength)},
      });
    }

    static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::PostProcess_SSAO & o) {
      mem::construct(&o);

      ar(s.get("bias"), o.bias);
      ar(s.get("radius"), o.radius);
      ar(s.get("strength"), o.strength);

      return true;
    }
  };

  template<>
  struct Serialization<components::PostProcess_SSR> {
    static SerializedObject write(engine::LevelSerializer::Writer & ar, components::PostProcess_SSR const & o) {
      return SerializedObject::MakeMap({
        {"maxDistance", ar(o.maxDistance)},
        {"resolution", ar(o.resolution)},
        {"steps", ar(o.steps)},
        {"thickness", ar(o.thickness)},
      });
    }

    static bool read(engine::LevelSerializer::Reader & ar, SerializedObject const & s, components::PostProcess_SSR & o) {
      mem::construct(&o);

      ar(s.get("maxDistance"), o.maxDistance);
      ar(s.get("resolution"), o.resolution);
      ar(s.get("steps"), o.steps);
      ar(s.get("thickness"), o.thickness);

      return true;
    }
  };
} // namespace bfc
