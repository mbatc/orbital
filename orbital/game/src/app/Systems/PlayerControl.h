#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"

namespace engine {
  class Input;
}

/// Camera controls for a "vehicle" camera rig.
/// Inputs are applied to the entity this component is attached to.
struct VehicleCameraController {
  engine::EntityID follow; ///< The target entity to follow
};

/// Camera controls for a "vehicle" camera rig.
/// Inputs are applied to the entity this component is attached to.
struct VehicleController {};

struct VehicleVelocity {
  bfc::Vec3d velocity;
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
      auto ret = bfc::SerializedObject::MakeMap();

      if (level.contains(o.follow)) {
        ret.add("follow", bfc::serialize(level.uuidOf(o.follow)));
      }

      return ret;
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, VehicleCameraController & o) {
      bfc::SerializedObject const & followGuid = s.get("follow");
      bfc::mem::construct(&o);
      o.follow = InvalidEntity;
      if (followGuid.isText()) {
        o.follow = level.find(bfc::UUID(followGuid.asText()));
      }

      return true;
    }
  };

  template<>
  struct LevelComponentSerializer<VehicleController> {
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, VehicleController const & o) {
      return bfc::SerializedObject::Empty();
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & s, Level & level, EntityID entity, VehicleController & o) {
      return true;
    }
  };
} // namespace engine
