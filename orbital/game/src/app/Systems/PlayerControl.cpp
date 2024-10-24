#include "PlayerControl.h"
#include "Input.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include "platform/KeyCode.h"
#include "Levels/CoreComponents.h"
#include "util/Log.h"

using namespace engine;
using namespace bfc;

PlayerControlSystem::PlayerControlSystem(bfc::Ref<engine::Input> const & pInput)
  : m_pInput(pInput) {
  pInput->channel("camera.look").mapButton({"mouse", MouseButton_Right});

  pInput->channel("camera.look.x")
    .setRange({-std::numeric_limits<float>::max(), std::numeric_limits<float>::max()})
    .mapAnalog({"mouse", MouseAxis_X, -1});

  pInput->channel("camera.look.y")
    .setRange({-std::numeric_limits<float>::max(), std::numeric_limits<float>::max()})
    .mapAnalog({"mouse", MouseAxis_Y, -1});

  pInput->channel("vehicle.thrust")
    .mapButton({"keyboard", KeyCode_Space, 1})
    .mapButton({"keyboard", KeyCode_Shift, -1});

  pInput->channel("vehicle.stop-engines").mapButton({"keyboard", KeyCode_X});

  pInput->channel("vehicle.roll")
    .mapButton({"keyboard", KeyCode_Q, -1})
    .mapButton({"keyboard", KeyCode_E, 1});

  pInput->channel("vehicle.yaw")
    .mapButton({"keyboard", KeyCode_A, -1})
    .mapButton({"keyboard", KeyCode_D, 1});

  pInput->channel("vehicle.pitch")
    .mapButton({"keyboard", KeyCode_W, -1})
    .mapButton({"keyboard", KeyCode_S, 1});
}

void PlayerControlSystem::update(engine::Level * pLevel, Timestamp dt) {
  float dtS = (float)dt.secs();
  for (auto & [controller] : pLevel->getView<VehicleController>()) {
    components::Transform * pTargetTransform = pLevel->tryGet<components::Transform>(controller.target);
    if (pTargetTransform == nullptr)
      continue;

    if (!pLevel->has<VehicleVelocity>(controller.target))
      pLevel->add<VehicleVelocity>(controller.target);

    VehicleVelocity & vel = pLevel->get<VehicleVelocity>(controller.target);

    controller.thrust += dtS * m_pInput->value("vehicle.thrust") * 5;
    controller.thrust  = bfc::math::max(0.0f, controller.thrust);
    if (m_pInput->pressed("vehicle.stop-engines")) {
      controller.thrust = 0.0f;
    }

    Vec3 up = glm::normalize(pTargetTransform->up());
    vel.velocity += dtS * up * controller.thrust;

    Quatd roll  = glm::angleAxis(dtS * m_pInput->value("vehicle.roll"), (Vec3)math::up<float>);
    Quatd yaw   = glm::angleAxis(dtS * m_pInput->value("vehicle.yaw"), (Vec3)math::forward<float>);
    Quatd pitch = glm::angleAxis(dtS * m_pInput->value("vehicle.pitch"), (Vec3)math::right<float>);

    pTargetTransform->rotate(roll);
    pTargetTransform->rotate(pitch);
    pTargetTransform->rotate(yaw);
  }

  for (auto & [transform, velocity] : pLevel->getView<components::Transform, VehicleVelocity>()) {
    transform.translate(velocity.velocity);
    velocity.velocity -= math::up<float> * 9.8f * (float)dt.secs();
    velocity.velocity -= velocity.velocity * math::clamp(1 * dt.secs(), 0.0, 1.0);
    if (transform.translation().y < 0) {
      velocity.velocity.y = 0;
      auto translation    = transform.translation();
      translation.y       = 0;
      transform.setTranslation(translation);
    }
  }

  for (auto & [transform, controller] : pLevel->getView<components::Transform, VehicleCameraController>()) {
    components::Transform * pTargetTransform = pLevel->tryGet<components::Transform>(controller.target);
    if (pTargetTransform == nullptr) {
      continue;
    }

    bfc::Vec3d targetPosition = pTargetTransform->globalTranslation(pLevel);
    transform.setGlobalTranslation(pLevel, targetPosition);

    if (m_pInput->down("camera.look")) {
      float yawRads   = glm::radians(m_pInput->change("camera.look.x"));
      float pitchRads = glm::radians(m_pInput->change("camera.look.y"));

      Quatd pitch = glm::angleAxis(pitchRads, math::right<float>);
      Quatd yaw   = glm::angleAxis(yawRads, math::up<float>);

      transform.rotate(pitch);
      transform.rotate(yaw);
    }

    // Maybe need to do this globally?
    transform.lookAt(transform.forward(), controller.up);
  }
}

void PlayerControlSystem::play(engine::Level * pLevel) {
  BFC_LOG_INFO("PlayerControlSystem", "play");
}

void PlayerControlSystem::pause(engine::Level * pLevel) {
  BFC_LOG_INFO("PlayerControlSystem", "pause");
}

void PlayerControlSystem::stop(engine::Level* pLevel) {
  BFC_LOG_INFO("PlayerControlSystem", "stop");
}
