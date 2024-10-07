#pragma once

#include "../../../vendor/imgui/imgui.h"
#include "../../../vendor/imgui/imgui_internal.h"
#include "../../../vendor/ImGuizmo/ImGuizmo.h"

#include "../render/GraphicsDevice.h"
#include "../render/Shader.h"

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
      void init(GraphicsDevice * pDevice);
      /// Shutdown the imgui context.
      void deinit();
      /// Called before every frame.
      /// @param size The size of the main window.
      void beginFrame(Vec2 size);
      /// Render ImGui draw data.
      void renderDrawData(ImDrawData * pDrawData);
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

    private:
      void updateMouseData();

      GraphicsDevice * m_pDevice        = nullptr;
      ImGuiContext *   m_pImContext     = nullptr;
      TextureAtlas *   m_pAtlas         = nullptr;
      bool             m_updateMonitors = true;
      GraphicsResource m_vertexArray    = InvalidGraphicsResource;
      GraphicsResource m_vertexBuffer   = InvalidGraphicsResource;
      GraphicsResource m_indexBuffer    = InvalidGraphicsResource;
      GraphicsResource m_fontTexture    = InvalidGraphicsResource;
      Shader           m_shader;
      GraphicsResource m_sampler         = InvalidGraphicsResource;
      ImGuiMouseCursor m_lastMouseCursor = ImGuiMouseCursor_None;
    };
    BFC_API CursorIcon getCursorIcon(ImGuiMouseCursor cursor);
  } // namespace ui
} // namespace bfc
