
#pragma once

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"
#include "Core/SerializedObject.h"

namespace components {
  /// Attach to the entity that should be controlled by the player.
  struct ThirdPersonCharacterController {
    engine::EntityID camera;

    struct Meta {
      static bfc::SerializedObject write(ThirdPersonCharacterController const & o, engine::ComponentSerializeContext const & ctx) {
        return bfc::SerializedObject::MakeMap({
          { "camera", ctx.pSerializer->writeEntityID(o.camera, *ctx.pLevel) }
        });
      }

      static bool read(bfc::SerializedObject const & s, ThirdPersonCharacterController & o,
                       engine::ComponentSerializeContext const & ctx) {
        engine::LevelSerializer::readEntityID(s.get("camera"), );

        s.get("camera").readOrConstruct(o.camera, engine::InvalidEntity);
        s.readOrConstruct(o.camera);
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
    : public engine::AutoRegisterLevelSystem<CharacterContollerLevelSystem>
    , public engine::ILevelUpdate {
    void update(engine::Level * pLevel, bfc::Timestamp dt) override;
  };

  extern CharacterContollerLevelSystem characterControllerLevelSystem;
}
