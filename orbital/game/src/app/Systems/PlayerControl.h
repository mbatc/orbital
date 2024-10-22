#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"

namespace engine {
  class Input;
}

/// Camera controls for a "vehicle" camera rig.
/// Inputs are applied to the entity this component is attached to.
struct VehicleCameraControls {

};

/// Inputs for a "vehicle" entity.
/// Inputs are applied to the entity this component is attached to.
struct VehicleControls {

};

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

namespace bfc {
  template<>
  struct Serializer<VehicleCameraControls> {
    inline static SerializedObject write(VehicleCameraControls const & o) {
      return SerializedObject::Empty();
    }

    inline static bool read(SerializedObject const & s, VehicleCameraControls & o) {
      return true;
    }
  };

  template<>
  struct Serializer<VehicleControls> {
    inline static SerializedObject write(VehicleControls const & o) {
      return SerializedObject::Empty();
    }

    inline static bool read(SerializedObject const & s, VehicleControls & o) {
      return true;
    }
  };
}
