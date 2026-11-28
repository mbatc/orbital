#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"
#include "Core/Serialize.h"

namespace components {
  /// Attach to the entity that should be controlled by the player.
  struct ThirdPersonCharacterController {
    engine::EntityID camera;
    bfc::Vec3d       cameraOffset;
    bfc::Vec3d       cameraForward;

    struct Meta {
      static bfc::SerializedObject write(engine::LevelSerializer::Writer & ar, ThirdPersonCharacterController const & o) {
        return bfc::SerializedObject::MakeMap({
          {"camera", ar(o.camera)},
          {"cameraOffset", ar(o.cameraOffset)},
          {"cameraForward", ar(o.cameraForward)},
        });
      }

      static bool read(engine::LevelSerializer::Reader & ar, bfc::SerializedObject const & s, ThirdPersonCharacterController & o) {
        ar(s.get("camera"), o.camera);
        ar(s.get("cameraOffset"), o.cameraOffset);
        ar(s.get("cameraForward"), o.cameraForward);
        return true;
      }

      static constexpr auto reflect() {
        return bfc::makeReflection<ThirdPersonCharacterController>(BFC_REFLECT(ThirdPersonCharacterController, camera));
      }
    };
  };
}

namespace engine {
  struct CharacterContollerLevelSystem
    : public engine::ILevelUpdate
    , public engine::ILevelActivate {
    void update(engine::Level * pLevel, bfc::Timestamp dt) override;
    void activate(Level * pLevel) override;
  };
}
