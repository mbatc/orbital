#pragma once

#include "input/Keyboard.h"
#include "input/Mouse.h"
#include "platform/Events.h"
#include "Rendering/RenderScene.h"

namespace bfc {
  class GraphicsDevice;
}

namespace engine {
  class Level;
  class DeferredRenderer;
  class AssetManager;
  class Viewport {
  public:
    Viewport(bfc::graphics::CommandList * pCmdList, AssetManager * pAssets, bfc::StringView const & viewportName);

    /// Render the viewport.
    void render(bfc::graphics::CommandList * pCmdList, bfc::graphics::RenderTargetRef renderTarget);

    /// Collect all of the views that need to be rendered
    virtual bfc::Vector<RenderView> collectViews(bfc::graphics::RenderTargetRef renderTarget) const = 0;

    /// Set the size of the viewport.
    void setSize(bfc::graphics::CommandList * pCmdList, bfc::Vec2i const & size);

    /// Set the Level this viewport should render.
    void setLevel(bfc::Ref<Level> const & pLevel);

    /// Get the Level this viewport is rendering.
    Level * getLevel() const;

    /// Get the size of the viewport.
    bfc::Vec2i getSize() const;

    /// Get the event system for this viewport.
    /// Input devices in this viewport are listening to this event queue.
    bfc::Events const * getEvents() const;
    bfc::Events *       getEvents();

    DeferredRenderer *    getRenderer() const;
    bfc::GraphicsDevice * getGraphics() const;

    /// Get the input devices exposed by this viewport.
    virtual bfc::Map<bfc::String, bfc::InputDevice*> getInputDevices() = 0;

  private:
    RenderScene m_renderScene;

    bfc::GraphicsDevice * m_pGraphics = nullptr;

    bfc::Vec2i m_size;
    bfc::Events m_events;

    bfc::Ref<DeferredRenderer> m_pRenderer;
  };
} // namespace engine
