#include "GameViewport.h"
#include "Levels/Level.h"
#include "Levels/CoreComponents.h"
#include "Rendering/DeferredRenderer.h"

using namespace bfc;

namespace engine {
  GameViewport::GameViewport(bfc::Ref<Renderer> const & pRenderer)
    : Viewport(pRenderer, "Game") {
    m_mouse.attachTo(getEvents());
    m_keyboard.attachTo(getEvents());
  }

  GameViewport::GameViewport(bfc::graphics::CommandList * pCmdList, AssetManager * pAssets)
    : GameViewport(bfc::NewRef<DeferredRenderer>(pCmdList, pAssets)) {}

  Vector<RenderView> GameViewport::collectViews(bfc::graphics::RenderTargetRef renderTarget) const {
    Level * pLevel = getLevel();

    Vector<RenderView> ret;
    for (auto & [transform, camera] : getLevel()->getView<components::Transform, components::Camera>()) {
      RenderView view;

      if (view.renderTarget == InvalidGraphicsResource) {
        view.renderTarget = renderTarget;
      }

      Vec2 renderSize = view.renderTarget->getSize();
      renderSize *= camera.viewportSize;

      view.projectionMatrix = camera.projectionMat(renderSize.x / renderSize.y);
      view.viewMatrix       = transform.globalTransformInverse(pLevel);

      if (camera.renderTarget != InvalidGraphicsResource) {
        view.renderTarget = camera.renderTarget;
      }

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
