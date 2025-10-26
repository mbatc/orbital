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
  struct Serializer<components::Transform> {
    inline static SerializedObject write(components::Transform const & o, engine::ComponentSerializeContext const & ctx) {
      auto ret = SerializedObject::MakeMap({{"translation", serialize(o.translation())},
                                                 {"ypr", serialize(glm::degrees(o.ypr()))},
                                                 {"scale", serialize(o.scale())},
                                                 {"parent", ctx.pSerializer->writeEntityID(o.parent(), *ctx.pLevel)}});

      if (ctx.pLevel->contains(o.parent())) {
        ret.add("parent", serialize(ctx.pLevel->uuidOf(o.parent())));
      }

      return ret;
    }

    inline static bool read(SerializedObject const & s, components::Transform & o, engine::ComponentDeserializeContext const & ctx) {
      Vec3d translation, ypr, scale;
      bfc::read(s.get("translation"), translation);
      bfc::read(s.get("ypr"), ypr);
      bfc::read(s.get("scale"), scale);

      mem::construct(&o);
      o.setTranslation(translation);
      o.setYpr(glm::radians(ypr));
      o.setScale(scale);

      engine::EntityID parentID = ctx.pSerializer->readEntityID(s.get("parent"), *ctx.pLevel);
      if (parentID != engine::InvalidEntity) {
        ctx.pSerializer->deferRead([entity = ctx.entity, parentID](engine::Level & level) {
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
  struct Serializer<components::ShadedMaterial> {
    // Default implementation delegates to a bfc serialize implementation for the component type.
    inline static SerializedObject write(components::ShadedMaterial const & o, engine::ComponentSerializeContext const & ctx) {
      return SerializedObject::MakeMap({
        {"material", serialize(o.pMaterial, ctx)},
        {"shader", serialize(o.pProgram, ctx)},
      });
    }

    inline static bool read(SerializedObject const & s, components::ShadedMaterial & o, engine::ComponentDeserializeContext const & ctx) {
      if (s.get("uri") || s.get("uuid")) {
        ctx.pSerializer->readAsync(ctx, &components::ShadedMaterial::pMaterial, s);
      } else {
        ctx.pSerializer->readAsync(ctx, &components::ShadedMaterial::pMaterial, s.get("material"));
        ctx.pSerializer->readAsync(ctx, &components::ShadedMaterial::pProgram, s.get("shader"));
        s.get("material").read(o.pMaterial, ctx);
        s.get("shader").read(o.pProgram, ctx);
      }
      return true;
    }
  };

  // A Specific serializer type that should be specialized for more control over entity component serialization.
  template<>
  struct Serializer<components::StaticMesh> {
    // Default implementation delegates to a bfc serialize implementation for the component type.
    inline static SerializedObject write(components::StaticMesh const & o, engine::ComponentSerializeContext const & ctx) {
      return SerializedObject::MakeMap({
        {"castShadows", serialize(o.castShadows, ctx)},
        {"useTesselation", serialize(o.useTesselation, ctx)},
        {"materials", serialize(o.materials, ctx)},
        {"mesh", serialize(o.pMesh, ctx)},
      });
    }

    inline static bool read(SerializedObject const & s, components::StaticMesh & o,
                            engine::ComponentDeserializeContext const & ctx) {
      s.get("castShadows").readOrConstruct(o.castShadows, true);
      s.get("useTesselation").readOrConstruct(o.useTesselation, false);

      ctx.pSerializer->readAsync(ctx, &components::StaticMesh::pMesh, s.get("mesh"));
      ctx.pSerializer->readAsync(ctx, &components::StaticMesh::materials, s.get("materials"));

      return true;
    }
  };

  template<>
  struct Serializer<components::Skybox> {
    inline static SerializedObject write(components::Skybox const & o, engine::ComponentSerializeContext const & ctx) {
      return SerializedObject::MakeMap({{"texture", ctx.pSerializer->writeAsset(o.pTexture)}});
    }

    inline static bool read(SerializedObject const & s, components::Skybox & o, engine::ComponentDeserializeContext const & ctx) {
      ctx.pSerializer->readAsync(ctx, &components::Skybox::pTexture, s.get("texture"));
      return true;
    }
  };

  template<>
  struct Serializer<components::PostProcess_Bloom> {
    inline static SerializedObject write(components::PostProcess_Bloom const & o, engine::ComponentSerializeContext const & ctx) {
      return SerializedObject::MakeMap({{"filterRadius", serialize(o.filterRadius, ctx)},
                                             {"strength", serialize(o.strength, ctx)},
                                             {"threshold", serialize(o.threshold, ctx)},
                                             {"dirtIntensity", serialize(o.dirtIntensity, ctx)},
                                             {"dirt", ctx.pSerializer->writeAsset(o.dirt)}});
    }

    inline static bool read(SerializedObject const & s, components::PostProcess_Bloom & o, engine::ComponentDeserializeContext const & ctx) {
      s.get("filterRadius").readOrConstruct(o.filterRadius, 0.005f);
      s.get("strength").read(o.strength, 0.04f);
      s.get("threshold").read(o.threshold, 0.25f);
      s.get("dirtIntensity").read(o.dirtIntensity, 1.0f);

      ctx.pSerializer->readAsync(ctx, &components::PostProcess_Bloom::dirt, s.get("dirt"));

      return true;
    }
  };

  template<>
  struct Serializer<components::Name> {
    template<typename Context>
    static SerializedObject write(components::Name const & o, Context const &) {
      return serialize(o.name);
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::Name & o, Context const &) {
      s.readOrConstruct(o.name);
      return true;
    }
  };

  template<>
  struct Serializer<components::Camera> {
    template<typename Context>
    static SerializedObject write(components::Camera const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"farPlane", serialize(o.farPlane)},
        {"nearPlane", serialize(o.nearPlane)},
        {"viewportSize", serialize(o.viewportSize)},
        {"viewportPosition", serialize(o.viewportPosition)},
        {"fov", serialize(glm::degrees(o.fov))}
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::Camera & o, Context const &) {
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
    template<typename Context>
    static SerializedObject write(components::Light const & o, Context const &) {
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

    template<typename Context>
    static bool read(SerializedObject const & s, components::Light & o, Context const &) {
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
    template<typename Context>
    static SerializedObject write(components::PostProcessVolume const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"enabled", serialize(o.enabled)},
        {"extents", serialize(o.extents)},
        {"infinite", serialize(o.infinite)},
        {"effects", serialize(o.effects)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::PostProcessVolume & o, Context const &) {
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
    template<typename Context>
    static SerializedObject write(components::PostProcess_Tonemap const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"exposure", serialize(o.exposure)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::PostProcess_Tonemap & o, Context const &) {
      mem::construct(&o);

      s.get("exposure").read(o.exposure);

      return true;
    }
  };

  template<>
  struct Serializer<components::PostProcess_SSAO> {
    template<typename Context>
    static SerializedObject write(components::PostProcess_SSAO const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"bias", serialize(o.bias)},
        {"radius", serialize(o.radius)},
        {"strength", serialize(o.strength)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::PostProcess_SSAO & o, Context const &) {
      mem::construct(&o);

      s.get("bias").read(o.bias);
      s.get("radius").read(o.radius);
      s.get("strength").read(o.strength);

      return true;
    }
  };

  template<>
  struct Serializer<components::PostProcess_SSR> {
    template<typename Context>
    static SerializedObject write(components::PostProcess_SSR const & o, Context const &) {
      return SerializedObject::MakeMap({
        {"maxDistance", serialize(o.maxDistance)},
        {"resolution", serialize(o.resolution)},
        {"steps", serialize(o.steps)},
        {"thickness", serialize(o.thickness)},
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::PostProcess_SSR & o, Context const &) {
      mem::construct(&o);

      s.get("maxDistance").read(o.maxDistance);
      s.get("resolution").read(o.resolution);
      s.get("steps").read(o.steps);
      s.get("thickness").read(o.thickness);

      return true;
    }
  };
} // namespace bfc
