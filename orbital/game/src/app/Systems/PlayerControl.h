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

  // Inherited via ILevelUpdate
  virtual void update(engine::Level * pLevel, bfc::Timestamp dt) override;
  virtual void play(engine::Level * pLevel) override;
  virtual void pause(engine::Level * pLevel) override;
  virtual void stop(engine::Level * pLevel) override;

private:
  bfc::Ref<engine::Input> m_pInput = nullptr;
};

namespace engine {
  template<>
  struct LevelComponentSerializer<VehicleCameraController> {
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, VehicleCameraController const & o) {
      return bfc::SerializedObject::MakeMap({{"target", LevelSerializer::writeEntityID(o.target, level)}, { "up", bfc::serialize(o.up) }});
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, VehicleCameraController & o) {
      bfc::mem::construct(&o);
      bfc::deserialize(s.get("up"), o.up);
      o.target = LevelSerializer::readEntityID(s.get("target"), level);
      return true;
    }
  };

  template<>
  struct engine::LevelComponentCopier<VehicleCameraController> {
    inline static void copy(LevelCopyContext * pContext, Level * pDstLevel, EntityID dstEntity, Level const & srcLevel,
                            VehicleCameraController const & component) {
      BFC_UNUSED(srcLevel);

      VehicleCameraController &dst = pDstLevel->replace<VehicleCameraController>(dstEntity, component);
      dst.target                   = pContext->remap(component.target);
    }
  };

  template<>
  struct LevelComponentSerializer<VehicleController> {
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, VehicleController const & o) {
      return bfc::SerializedObject::MakeMap({{"target", LevelSerializer::writeEntityID(o.target, level)}});
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, VehicleController & o) {
      bfc::mem::construct(&o);
      o.target = LevelSerializer::readEntityID(s.get("target"), level);
      return true;
    }
  };

  template<>
  struct engine::LevelComponentCopier<VehicleController> {
    inline static void copy(LevelCopyContext * pContext, Level * pDstLevel, EntityID dstEntity, Level const & srcLevel, VehicleController const & component) {
      BFC_UNUSED(srcLevel);

      VehicleController & dst = pDstLevel->replace<VehicleController>(dstEntity, component);
      dst.target              = pContext->remap(component.target);
    }
  };

  template<>
  struct LevelComponentSerializer<Vehicle> {
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, Vehicle const & o) {
      return bfc::SerializedObject::Empty();
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, Vehicle & o) {
      return true;
    }
  };
} // namespace engine
