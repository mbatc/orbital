#include "LevelEditorViewport.h"
#include "input/Keyboard.h"
#include "input/Mouse.h"
#include "platform/KeyCode.h"
#include "Rendering/DeferredRenderer.h"
using namespace bfc;

namespace engine {
  EditorCamera::EditorCamera(Mouse * pMouse, Keyboard * pKeyboard)
    : m_pMouse(pMouse)
    , m_pKeyboard(pKeyboard) {}

  void EditorCamera::update(Timestamp dt) {
    if (m_pMouse->isDown(MouseButton_Right)) {
      Vec2d change = m_pMouse->getPositionChange() / 100;
      m_ypr.x -= change.x;
      m_ypr.y -= change.y;

      if (!m_hasMouse) {
        setCursorVisible(false);
        m_hasMouse = true;
      }

      float scrollAmt = m_pMouse->getScroll();
      float mult      = std::pow(2.0f, scrollAmt);
      speedMultiplier *= mult;
    } else {
      if (m_hasMouse) {
        setCursorVisible(true);
        m_hasMouse = false;
      }
    }

    Vec3d mv = Vec3d(0);
    if (m_pKeyboard->isDown(KeyCode_W)) {
      mv.z -= 0.05;
    }

    if (m_pKeyboard->isDown(KeyCode_S)) {
      mv.z += 0.05;
    }

    if (m_pKeyboard->isDown(KeyCode_D)) {
      mv.x += 0.05;
    }

    if (m_pKeyboard->isDown(KeyCode_A)) {
      mv.x -= 0.05;
    }

    if (m_pKeyboard->isDown(KeyCode_E)) {
      mv.y += 0.05;
    }

    if (m_pKeyboard->isDown(KeyCode_X)) {
      mv.y -= 0.05;
    }

    float finalMutlipler = speedMultiplier;

    if (m_pKeyboard->isDown(KeyCode_Shift)) {
      finalMutlipler *= 2;
    }

    if (m_pKeyboard->isDown(KeyCode_Control)) {
      finalMutlipler /= 2;
    }

    m_ypr = glm::mod(m_ypr, 2 * glm::pi<double>());

    Mat4 camRot = glm::yawPitchRoll(m_ypr.x, m_ypr.y, 0.0);
    mv          = camRot * Vec4d(mv, 0);
    m_position  = m_position + mv * (double)finalMutlipler;
  }

  void EditorCamera::setFOV(float fov) {
    m_fov = fov;
  }

  void EditorCamera::setNearPlane(float nearPlane) {
    m_nearPlane = nearPlane;
  }

  void EditorCamera::setFarPlane(float farPlane) {
    m_farPlane = farPlane;
  }

  float EditorCamera::fov() const {
    return m_fov;
  }

  float EditorCamera::nearPlane() const {
    return m_nearPlane;
  }

  float EditorCamera::farPlane() const {
    return m_farPlane;
  }

  Mat4d EditorCamera::transformMat() const {
    return math::translation(m_position) * math::rotationYPR(m_ypr);
  }

  Mat4d EditorCamera::viewMat() const {
    return glm::inverse(transformMat());
  }

  Mat4d EditorCamera::projectionMat(double aspect) const {
    return glm::perspective<double>(m_fov, aspect, m_nearPlane, m_farPlane);
  }

  Mat4d EditorCamera::viewProjectionMat(double aspect) const {
    return projectionMat(aspect) * viewMat();
  }

  LevelEditorViewport::LevelEditorViewport(bfc::Ref<Renderer> const & pRenderer)
    : Viewport(pRenderer, "LevelEditor")
    , camera(&m_mouse, &m_keyboard) {
    m_mouse.attachTo(getEvents());
    m_keyboard.attachTo(getEvents());
  }

  LevelEditorViewport::LevelEditorViewport(bfc::graphics::CommandList * pCmdList, AssetManager * pAssets) 
    : LevelEditorViewport(bfc::NewRef<DeferredRenderer>(pCmdList, pAssets)) {}

  Vector<RenderView> LevelEditorViewport::collectViews(bfc::graphics::RenderTargetRef renderTarget) const {
    RenderView view;
    view.viewport         = {0, 0, 1, 1};
    view.viewMatrix       = camera.viewMat();
    view.projectionMatrix = camera.projectionMat((float)getSize().x / getSize().y);
    view.renderTarget     = renderTarget;
    return { view };
  }

  bfc::Map<bfc::String, bfc::InputDevice *> LevelEditorViewport::getInputDevices() {
    return {
      {"keyboard", &m_keyboard},
      {"mouse",    &m_mouse},
    };
  }

  const bfc::Mouse & LevelEditorViewport::getMouse() const {
    return m_mouse;
  }

  const bfc::Keyboard & LevelEditorViewport::getKeyboard() const {
    return m_keyboard;
  }

  bfc::Mouse & LevelEditorViewport::getMouse() {
    return m_mouse;
  }

  bfc::Keyboard & LevelEditorViewport::getKeyboard() {
    return m_keyboard;
  }
} // namespace engine
