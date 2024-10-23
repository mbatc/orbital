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
  pInput->channel("look").mapButton({"mouse", MouseButton_Right});

  pInput->channel("look.x")
    .setRange({-std::numeric_limits<float>::max(), std::numeric_limits<float>::max()})
    .mapAnalog({"mouse", MouseAxis_X});

  pInput->channel("look.y")
    .setRange({-std::numeric_limits<float>::max(), std::numeric_limits<float>::max()})
    .mapAnalog({"mouse", MouseAxis_Y});

  pInput->channel("thrust.increase").mapButton({"keyboard", KeyCode_Space});
  pInput->channel("thrust.decrease").mapButton({"keyboard", KeyCode_Shift});

  pInput->channel("control.roll").mapButton({"keyboard", KeyCode_Q, 1}).mapButton({"keyboard", KeyCode_E, -1});
  pInput->channel("control.yaw").mapButton({"keyboard", KeyCode_A, 1}).mapButton({"keyboard", KeyCode_D, -1});
  pInput->channel("control.pitch").mapButton({"keyboard", KeyCode_W, 1}).mapButton({"keyboard", KeyCode_S, -1});
}

void PlayerControlSystem::update(engine::Level * pLevel, Timestamp dt) {
  float dtS = (float)dt.secs();
  for (auto & [transform, controller] : pLevel->getView<components::Transform, VehicleController>()) {
    EntityID id = pLevel->toEntity(&transform);

    if (!pLevel->has<VehicleVelocity>(id))
      pLevel->add<VehicleVelocity>(id);

    VehicleVelocity & vel = pLevel->get<VehicleVelocity>(id);

    Vec3 up = glm::normalize(transform.up());
    vel.velocity += dtS * up * (m_pInput->value("thrust.increase") - m_pInput->value("thrust.decrease")) * 9.8 * 2;

    Quatd roll  = glm::angleAxis(dtS * m_pInput->value("control.roll"), (Vec3)math::up<float>);
    Quatd yaw   = glm::angleAxis(dtS * m_pInput->value("control.yaw"), (Vec3)math::forward<float>);
    Quatd pitch = glm::angleAxis(dtS * m_pInput->value("control.pitch"), (Vec3)math::right<float>);

    transform.rotate(roll);
    transform.rotate(pitch);
    transform.rotate(yaw);
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
    components::Transform * pTargetTransform = pLevel->tryGet<components::Transform>(controller.follow);
    if (pTargetTransform == nullptr) {
      continue;
    }

    bfc::Vec3d targetPosition = pTargetTransform->globalTranslation(pLevel);
    transform.setGlobalTranslation(pLevel, targetPosition);

    if (m_pInput->down("look")) {
      float yawRads   = glm::radians(m_pInput->change("look.x"));
      float pitchRads = glm::radians(m_pInput->change("look.y"));

      Quatd pitch = glm::angleAxis(pitchRads, math::right<float>);
      Quatd yaw   = glm::angleAxis(yawRads, math::up<float>);

      transform.rotate(pitch);
      transform.rotate(yaw);
    }
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
