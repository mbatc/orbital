#pragma once

#include "input/Keyboard.h"
#include "Viewport.h"

namespace engine {
  class Level;
  class Renderer;
  class AssetManager;
  class GameViewport : public Viewport {
  public:
    GameViewport(bfc::Ref<Renderer> const & pRenderer);
    GameViewport(bfc::graphics::CommandList * pCmdList, AssetManager * pAssets);

    virtual bfc::Vector<RenderView> collectViews(bfc::graphics::RenderTargetRef renderTarget) const override;

    virtual bfc::Map<bfc::String, bfc::InputDevice *> getInputDevices() override;

    const bfc::Mouse &    getMouse() const;
    const bfc::Keyboard & getKeyboard() const;

    bfc::Mouse &    getMouse();
    bfc::Keyboard & getKeyboard();

  private:
    bfc::Mouse    m_mouse;
    bfc::Keyboard m_keyboard;
  };
} // namespace engine
