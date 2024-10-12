#pragma once

#include "input/Keyboard.h"
#include "input/Mouse.h"
#include "platform/Events.h"
#include "render/Texture.h"
#include "Renderer/RenderScene.h"

namespace bfc {
  class GraphicsDevice;
}

namespace engine {
  class Level;
  class DeferredRenderer;
  class AssetManager;
  class Viewport {
  public:
    Viewport(bfc::GraphicsDevice * pGraphics, AssetManager * pAssets, bfc::StringView const & viewportName);

    /// Render the viewport.
    void render();

    /// Set the size of the viewport.
    void setSize(bfc::Vec2i const & size);

    void setLevel(bfc::Ref<Level> const & pLevel);

    /// Get the size of the viewport.
    bfc::Vec2i getSize() const;

    const bfc::Mouse &    getMouse() const;
    const bfc::Keyboard & getKeyboard() const;

    bfc::Mouse &    getMouse();
    bfc::Keyboard & getKeyboard();

    /// Get the event system for this viewport.
    /// Input devices in this viewport are listening to this event queue.
    bfc::Events const * getEvents() const;
    bfc::Events *       getEvents();

    void setView(bfc::Mat4d const & viewMatrix);
    void setProjection(bfc::Mat4d const & projectionMatrix);

    bfc::Mat4d getView() const;
    bfc::Mat4d getProjection() const;

    bfc::GraphicsResource const & getRenderTarget() const;
    bfc::Texture const &          getColourTexture() const;
    bfc::Texture const &          getDepthTexture() const;

    DeferredRenderer *    getRenderer() const;
    bfc::GraphicsDevice * getGraphics() const;

  private:
    RenderScene m_renderScene;

    bfc::GraphicsDevice * m_pGraphics = nullptr;

    bfc::Vec2i m_size;

    bfc::Events   m_events;
    bfc::Keyboard m_keyboard;
    bfc::Mouse    m_mouse;

    bfc::Ref<DeferredRenderer> m_pRenderer;

    bfc::GraphicsResource m_renderTarget = bfc::InvalidGraphicsResource;
    bfc::Texture          m_colourTexture;
    bfc::Texture          m_depthTexture;

    bfc::Mat4d m_viewMatrix       = bfc::math::identity<bfc::Mat4d>();
    bfc::Mat4d m_projectionMatrix = bfc::math::identity<bfc::Mat4d>();
  };
} // namespace engine
