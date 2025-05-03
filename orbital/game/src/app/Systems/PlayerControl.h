#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"

namespace engine {
  class Input;
}

/// Camera controls for a "vehicle" camera rig.
/// Inputs are applied to the entity this component is attached to.
struct VehicleCameraController {
  engine::EntityID target;             ///< The target entity to follow.
  bfc::Vec3d       up = bfc::math::up<double>; ///< Up direction to fix the camera to.
};

/// Camera controls for a "vehicle" camera rig.
/// Inputs are applied to the entity this component is attached to.
struct VehicleController {
  engine::EntityID target;        ///< The target entity to control
  float            thrust = 0.0f;
};

/// Vehicle tag component.
/// May contain more details at some point.
struct Vehicle {};

struct VehicleVelocity {
  bfc::Vec3d velocity = bfc::Vec3d(0);
};

class PlayerControlSystem
  : public engine::ILevelUpdate
  , public engine::ILevelPlay
  , public engine::ILevelPause
  , public engine::ILevelStop {
public:
  PlayerControlSystem(bfc::Ref<engine::Input> const & pInput);

  virtual void update(engine::Level * pLevel, bfc::Timestamp dt) override;
  virtual void play(engine::Level * pLevel) override;
  virtual void pause(engine::Level * pLevel) override;
  virtual void stop(engine::Level * pLevel) override;

private:
  bfc::Ref<engine::Input> m_pInput = nullptr;
};

namespace engine {
  template<>
  struct engine::LevelComponent_OnCopy<VehicleCameraController> {
    inline static void onCopy(LevelCopyContext * pContext, Level * pDstLevel, EntityID dstEntity, Level const & srcLevel,
                            VehicleCameraController const & component) {
      BFC_UNUSED(srcLevel);

      VehicleCameraController &dst = pDstLevel->replace<VehicleCameraController>(dstEntity, component);
      dst.target                   = pContext->remap(component.target);
    }
  };

  template<>
  struct engine::LevelComponent_OnCopy<VehicleController> {
    inline static void onCopy(LevelCopyContext * pContext, Level * pDstLevel, EntityID dstEntity, Level const & srcLevel, VehicleController const & component) {
      BFC_UNUSED(srcLevel);

      VehicleController & dst = pDstLevel->replace<VehicleController>(dstEntity, component);
      dst.target              = pContext->remap(component.target);
    }
  };
} // namespace engine

namespace bfc {
  template<>
  struct Serializer<VehicleCameraController> {
    static bfc::SerializedObject write(VehicleCameraController const & o, engine::ComponentSerializeContext const & ctx) {
      return bfc::SerializedObject::MakeMap({{"target", ctx.pSerializer->writeEntityID(o.target, *ctx.pLevel)}, {"up", bfc::serialize(o.up)}});
    }

    static bool read(bfc::SerializedObject const & s, VehicleCameraController & o, engine::ComponentDeserializeContext const & ctx) {
      bfc::mem::construct(&o);
      bfc::read(s.get("up"), o.up, ctx);
      o.target = ctx.pSerializer->readEntityID(s.get("target"), *ctx.pLevel);
      return true;
    }
  };

  template<>
  struct Serializer<VehicleController> {
    inline static bfc::SerializedObject write(VehicleController const & o, engine::ComponentSerializeContext const & ctx) {
      return bfc::SerializedObject::MakeMap({{"target", ctx.pSerializer->writeEntityID(o.target, *ctx.pLevel)}});
    }

    inline static bool read(bfc::SerializedObject const & s, VehicleController & o, engine::ComponentDeserializeContext const & ctx) {
      bfc::mem::construct(&o);
      o.target = ctx.pSerializer->readEntityID(s.get("target"), *ctx.pLevel);
      return true;
    }
  };

  template<>
  struct Serializer<Vehicle> {
    inline static bfc::SerializedObject write(Vehicle const & o,
                                              engine::ComponentSerializeContext const & ctx) {
      return bfc::SerializedObject::Empty();
    }

    inline static bool read(bfc::SerializedObject const & s, Vehicle & o, engine::ComponentDeserializeContext const & ctx) {
      mem::construct(&o);
      return true;
    }
  };
} // namespace bfc
