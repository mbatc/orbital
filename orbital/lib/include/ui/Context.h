#pragma once

#include "../../../vendor/imgui/imgui.h"
#include "../../../vendor/imgui/imgui_internal.h"
#include "../../../vendor/ImGuizmo/ImGuizmo.h"
#include "../render/GraphicsDevice.h"
#include "../render/Shader.h"
#include "../platform/Events.h"

// ImGui helpers should go here
namespace bfc {
  enum CursorIcon;
  class TextureAtlas;

  namespace ui {
    class BFC_API Context {
    public:
      Context();
      ~Context();

      /// Initialise the imgui context using the given GraphicsDevice.
      void init(graphics::CommandList * pCmdList);
      /// Shutdown the imgui context.
      void deinit();
      /// Called before every frame.
      /// @param size The size of the main window.
      void beginFrame(Vec2 size);
      /// Render ImGui draw data.
      void renderDrawData(graphics::CommandList * pCmdList, ImDrawData * pDrawData);
      /// Make this the current ImGui context.
      void bind();
      /// Get the graphics device used by the GUI context.
      GraphicsDevice * getDevice() const;
      /// Update the monitor list on the next update.
      void requestUpdateMonitors();
      /// Get the texture atlas used by this context.
      TextureAtlas * getTextureAtlas();
      /// Get the current Context.
      /// This is shorthand for from (Context*)ImGui::GetIO().UserData
      static Context * Current();

      Events * getEvents() {
        return &m_events;
      }

      Events const * getEvents() const {
        return &m_events;
      }

      /// Track a texture.
      /// Release after rendering this frame.
      uint64_t track(graphics::TextureRef texture);

    private:
      void updateMouseData();

      Events m_events;
      Ref<EventListener> m_pListener = nullptr;

      ImGuiContext *   m_pImContext     = nullptr;
      TextureAtlas *   m_pAtlas         = nullptr;
      bool             m_updateMonitors = true;

      graphics::VertexArrayRef m_vertexArray    = InvalidGraphicsResource;
      graphics::BufferRef      m_vertexBuffer   = InvalidGraphicsResource;
      graphics::BufferRef      m_indexBuffer    = InvalidGraphicsResource;
      graphics::TextureRef     m_fontTexture    = InvalidGraphicsResource;
      graphics::ProgramRef     m_shader;
      graphics::SamplerRef     m_sampler         = InvalidGraphicsResource;
      ImGuiMouseCursor         m_lastMouseCursor = ImGuiMouseCursor_None;

      Vector<graphics::TextureRef> m_tracked;
    };
    BFC_API CursorIcon getCursorIcon(ImGuiMouseCursor cursor);
  } // namespace ui
} // namespace bfc
