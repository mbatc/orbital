#include "Levels/CoreComponents.h"
#include "CharacterControllers.h"
#include "Input.h"
#include "Input/Keyboard.h"
#include "platform/KeyCode.h"

using namespace components;

namespace engine {
  void CharacterContollerLevelSystem::update(engine::Level * pLevel, bfc::Timestamp dt) {
    for (auto& [controller, transform] :
      pLevel->getView<ThirdPersonCharacterController, Transform>()) {

      // Move character
      const bfc::Vec3d velocity = bfc::Vec3d(
        inputs::value("move.left") + inputs::value("move.right"),
        0,
        inputs::value("move.forward") + inputs::value("move.backward"));

      const double speed = inputs::down("move.sprint") ? 2 : inputs::down("move.sneak") ? 0.5 : 1;
      const bfc::Vec3d localVelocity = Transform::applyToDirection(transform.transform(), velocity);

      transform.translate(localVelocity);

      // Move camera
      // auto &cameraTransform = pLevel->get<components::Transform>(controller.camera);
    }
  }

  void CharacterContollerLevelSystem::activate(Level *) {
    inputs::channel("move.left").mapButton(InputDescriptor{"keyboard", KeyCode_A, -1});
    inputs::channel("move.right").mapButton(InputDescriptor{"keyboard", KeyCode_D, 1});
    inputs::channel("move.forward").mapButton(InputDescriptor{"keyboard", KeyCode_W, 1});
    inputs::channel("move.backward").mapButton(InputDescriptor{"keyboard", KeyCode_S, -1});
    inputs::channel("move.sprint").mapButton(InputDescriptor{"keyboard", KeyCode_Shift, -1});
    inputs::channel("move.sneak").mapButton(InputDescriptor{"keyboard", KeyCode_Control, -1});
  }
}
