#pragma once

#include "Level.h"
#include "core/Vector.h"
#include "geometry/Geometry.h"
#include "math/MathTypes.h"
#include "util/UUID.h"
#include "render/GraphicsDevice.h"

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

  struct StaticMesh {
    bfc::Ref<bfc::Mesh>                  pMesh;
    bfc::Vector<bfc::Ref<bfc::Material>> materials;
    bool                                 castShadows = true;
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
  struct engine::LevelComponentSerializer<components::Transform> {
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, components::Transform const & o) {
      auto ret = bfc::SerializedObject::MakeMap({{"translation", bfc::serialize(o.translation())},
                                                 {"ypr", bfc::serialize(glm::degrees(o.ypr()))},
                                                 {"scale", bfc::serialize(o.scale())},
                                                 {"parent", LevelSerializer::writeEntityID(o.parent(), level)}});

      if (level.contains(o.parent())) {
        ret.add("parent", bfc::serialize(level.uuidOf(o.parent())));
      }

      return ret;
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, components::Transform & o) {
      bfc::Vec3d translation, ypr, scale;
      deserialize(s.get("translation"), translation);
      deserialize(s.get("ypr"), ypr);
      deserialize(s.get("scale"), scale);

      bfc::mem::construct(&o);
      o.setTranslation(translation);
      o.setYpr(glm::radians(ypr));
      o.setScale(scale);

      EntityID parentID = LevelSerializer::readEntityID(s.get("parent"), level);
      if (parentID != InvalidEntity) {
        pSerializer->deferRead([entity, parentID](Level & level) {
          components::Transform * pTransform = level.tryGet<components::Transform>(entity);
          if (pTransform != nullptr) {
            pTransform->setParent(&level, parentID);
          }
        });
      }

      return true;
    }
  };

  template<>
  struct engine::LevelComponentCopier<components::Transform> {
    inline static void copy(LevelCopyContext * pContext, Level * pDstLevel, EntityID dstEntity, Level const & srcLevel,
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

  // A Specific serializer type that should be specialized for more control over entity component serialization.
  template<>
  struct engine::LevelComponentSerializer<components::StaticMesh> {
    // Default implementation delegates to a bfc serialize implementation for the component type.
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, components::StaticMesh const & o) {
      BFC_UNUSED(pSerializer, level);

      return bfc::SerializedObject::MakeMap({
        {"castShadows", bfc::serialize(o.castShadows)},
        {"materials", pSerializer->writeAssets(o.materials)},
        {"mesh", pSerializer->writeAsset(o.pMesh)},
      });
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, components::StaticMesh & o) {
      BFC_UNUSED(pSerializer, level);

      s.get("castShadows").read(o.castShadows);

      pSerializer->readAsset(s.get("mesh"), o.pMesh);
      pSerializer->readAssets(s.get("materials"), o.materials);

      return true;
    }
  };

  template<>
  struct engine::LevelComponentSerializer<components::Skybox> {
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, components::Skybox const & o) {
      return bfc::SerializedObject::MakeMap({{"texture", pSerializer->writeAsset(o.pTexture)}});
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, components::Skybox & o) {
      pSerializer->readAsset(s.get("texture"), o.pTexture);

      return true;
    }
  };

  template<>
  struct engine::LevelComponentSerializer<components::PostProcess_Bloom> {
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, components::PostProcess_Bloom const & o) {
      return bfc::SerializedObject::MakeMap({{"filterRadius", bfc::serialize(o.filterRadius)},
                                             {"strength", bfc::serialize(o.strength)},
                                             {"threshold", bfc::serialize(o.threshold)},
                                             {"dirtIntensity", bfc::serialize(o.dirtIntensity)},
                                             {"dirt", pSerializer->writeAsset(o.dirt)}});
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, components::PostProcess_Bloom & o) {
      bfc::mem::construct(&o);

      s.get("filterRadius").read(o.filterRadius);
      s.get("strength").read(o.strength);
      s.get("threshold").read(o.threshold);
      s.get("dirtIntensity").read(o.dirtIntensity);

      pSerializer->readAsset(s.get("dirt"), o.dirt);

      return true;
    }
  };
} // namespace engine

namespace bfc {
  template<>
  struct Serializer<components::Name> {
    inline static SerializedObject write(components::Name const & o) {
      return serialize(o.name);
    }

    inline static bool read(SerializedObject const & s, components::Name & o) {
      s.readOrConstruct(o.name);
      return true;
    }
  };

  template<>
  struct Serializer<components::Camera> {
    inline static SerializedObject write(components::Camera const & o) {
      return SerializedObject::MakeMap({
        {"farPlane", serialize(o.farPlane)},
        {"nearPlane", serialize(o.nearPlane)},
        {"viewportSize", serialize(o.viewportSize)},
        {"viewportPosition", serialize(o.viewportPosition)},
        {"fov", serialize(glm::degrees(o.fov))}
      });
    }

    inline static bool read(SerializedObject const & s, components::Camera & o) {
      mem::construct(&o);

      s.get("farPlane").read(o.farPlane);
      s.get("nearPlane").read(o.nearPlane);
      s.get("viewportSize").read(o.viewportSize);
      s.get("viewportPosition").read(o.viewportPosition);
      s.get("fov").read(o.fov);

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
  struct Serializer<components::Light> {
    inline static SerializedObject write(components::Light const & o) {
      return SerializedObject::MakeMap({
        {"type", serialize(o.type)},
        {"colour", serialize(o.colour)},
        {"ambient", serialize(o.ambient)},
        {"attenuation", serialize(o.attenuation)},
        {"strength", serialize(o.strength)},
        {"innerConeAngle", serialize(glm::degrees(o.innerConeAngle))},
        {"outerConeAngle", serialize(glm::degrees(o.outerConeAngle))},
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

      o.innerConeAngle = glm::radians(o.innerConeAngle);
      o.outerConeAngle = glm::radians(o.outerConeAngle);

      return true;
    }
  };

  template<>
  struct Serializer<components::PostProcessVolume> {
    inline static SerializedObject write(components::PostProcessVolume const & o) {
      return SerializedObject::MakeMap({
        {"enabled", serialize(o.enabled)},
        {"extents", serialize(o.extents)},
        {"infinite", serialize(o.infinite)},
        {"effects", serialize(o.effects)},
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
} // namespace bfc
