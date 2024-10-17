#include "GameViewport.h"
#include "Levels/Level.h"
#include "Levels/CoreComponents.h"

using namespace bfc;

namespace engine {
  GameViewport::GameViewport(GraphicsDevice * pGraphics, AssetManager * pAssets)
    : Viewport(pGraphics, pAssets, "Game")
  {}

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
    }

    return ret;
  }
} // namespace engine
