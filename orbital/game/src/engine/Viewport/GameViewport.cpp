#include "GameViewport.h"
#include "Levels/Level.h"
#include "Levels/CoreComponents.h"

using namespace bfc;

namespace engine {
  GameViewport::GameViewport(GraphicsDevice * pGraphics, AssetManager * pAssets)
    : Viewport(pGraphics, pAssets, "Game") {
    m_mouse.attachTo(getEvents());
    m_keyboard.attachTo(getEvents());
  }

  Vector<RenderView> GameViewport::collectViews(GraphicsResource renderTarget) const {
    Level * pLevel = getLevel();
    bfc::graphics::RenderTargetManager *pRT = getGraphics()->getRenderTargetManager();

    Vector<RenderView> ret;
    for (auto & [transform, camera] : getLevel()->getView<components::Transform, components::Camera>()) {
      RenderView view;

      if (view.renderTarget == InvalidGraphicsResource) {
        view.renderTarget = renderTarget;
      }

      Vec2 renderSize = pRT->getSize(view.renderTarget);
      renderSize *= camera.viewportSize;

      view.projectionMatrix = camera.projectionMat(renderSize.x / renderSize.y);
      view.viewMatrix       = transform.globalTransformInverse(pLevel);
      view.renderTarget     = camera.renderTarget;
      view.viewport         = { camera.viewportPosition, camera.viewportSize };

      ret.pushBack(view);
    }

    return ret;
  }

  bfc::Map<bfc::String, bfc::InputDevice *> GameViewport::getInputDevices() {
    return {
      {"keyboard", &m_keyboard},
      {"mouse", &m_mouse},
    };
  }

  const bfc::Mouse & GameViewport::getMouse() const {
    return m_mouse;
  }

  const bfc::Keyboard & GameViewport::getKeyboard() const {
    return m_keyboard;
  }

  bfc::Mouse & GameViewport::getMouse() {
    return m_mouse;
  }

  bfc::Keyboard & GameViewport::getKeyboard() {
    return m_keyboard;
  }
} // namespace engine
