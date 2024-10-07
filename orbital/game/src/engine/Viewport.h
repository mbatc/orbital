#pragma once

#include "input/Mouse.h"
#include "input/Keyboard.h"
#include "platform/Events.h"
#include "render/Texture.h"

namespace engine {
  class DeferredRenderer;
  class Viewport {
  public:
    Viewport();
  
    /// Render the viewport.
    void render();
  
    /// Set the size of the viewport.
    void setSize(bfc::Vec2i const & size);
  
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
  
  private:
    bfc::Vec2i m_size;
  
    bfc::Events   m_events;
    bfc::Keyboard m_keyboard;
    bfc::Mouse    m_mouse;
  
    bfc::Ref<DeferredRenderer> m_pRenderer;
  
    bfc::GraphicsResource m_renderTarget = InvalidGraphicsResource;
    bfc::Texture          m_colourTexture;
    bfc::Texture          m_depthTexture;
  
    bfc::Mat4d m_viewMatrix       = bfc::math::identity<bfc::Mat4d>();
    bfc::Mat4d m_projectionMatrix = bfc::math::identity<bfc::Mat4d>();
  };
} // namespace engine
