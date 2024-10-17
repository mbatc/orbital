#pragma once

#include "input/Keyboard.h"
#include "Viewport.h"

namespace engine {
  class Level;
  class DeferredRenderer;
  class AssetManager;
  class GameViewport : public Viewport {
  public:
    GameViewport(bfc::GraphicsDevice * pGraphics, AssetManager * pAssets);

    virtual bfc::Vector<RenderView> collectViews(bfc::GraphicsResource renderTarget) const override;
  };
} // namespace engine
