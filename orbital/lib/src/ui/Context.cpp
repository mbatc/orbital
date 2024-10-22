#include "ui/Context.h"
#include "core/Timestamp.h"
#include "input/Mouse.h"
#include "platform/Display.h"
#include "platform/Window.h"
#include "render/TextureAtlas.h"

namespace bfc {
  static const char * g_vertexShaderSrc = "#version 430\n"
                                          "layout (location = 0) in vec2 Position;\n"
                                          "layout (location = 1) in vec4 Color;\n"
                                          "layout (location = 2) in vec2 UV;\n"
                                          "uniform mat4 ProjMtx;\n"
                                          "out vec2 Frag_UV;\n"
                                          "out vec4 Frag_Color;\n"
                                          "void main()\n"
                                          "{\n"
                                          "    Frag_UV = UV;\n"
                                          "    Frag_Color = Color;\n"
                                          "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
                                          "}\n";

  static const char * g_fragmentShaderSrc = "#version 430\n"
                                            "in vec2 Frag_UV;\n"
                                            "in vec4 Frag_Color;\n"
                                            "uniform sampler2D Texture;\n"
                                            "layout (location = 0) out vec4 Out_Color;\n"
                                            "void main()\n"
                                            "{\n"
                                            "    // mat3 coeff = mat3(0.025, 0.05, 0.025,"
                                            "0.05,   0.6, 0.05,"
                                            "0.025, 0.05, 0.025);\n"
                                            "    // mat3 coeff = mat3(0.025, 0.1, 0.025,"
                                            "0.1,   0.5, 0.1,"
                                            "0.025, 0.1, 0.025);\n"
                                            "    Out_Color = Frag_Color * textureLod(Texture, Frag_UV.st, 0);\n"
                                            "    // vec2 step = vec2(1.0) / textureSize(Texture, 0);\n"
                                            "    // for (int y = 0; y <= 2; ++y)\n"
                                            "    //   for (int x = 0; x <= 2; ++x)\n"
                                            "    //     Out_Color += Frag_Color * texture(Texture, Frag_UV.st + step * vec2(x - 1, y - 1)) * coeff[y][x];\n"
                                            "}\n";

  namespace ui {
    static ImGuiKey         translateKeyCode(KeyCode key);
    static ImGuiMouseButton translateMouseCode(MouseButton bt);

    Context::Context()
      : m_events("UI Context") {}
    Context::~Context() {}

    void Context::init(GraphicsDevice * pDevice) {
      m_pDevice    = pDevice;
      m_pAtlas     = new TextureAtlas(pDevice, Vec2i(32), Vec2i(16));

      m_pImContext = ImGui::CreateContext();
      m_pImContext->IO.UserData = this;

      ImGui::SetCurrentContext(m_pImContext);
      ImGuiIO & io = ImGui::GetIO();

      // Setup backend capabilities flags
      Timestamp ts               = Timestamp::now();
      io.BackendPlatformUserData = nullptr;
      io.BackendPlatformName     = "imgui_impl_bfc";
      io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
      io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
      // io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
      io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
      // io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

      io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
      // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
      // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
      // io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
      io.ConfigViewportsNoDecoration = true;

      graphics::BufferManager * pBuffers = pDevice->getBufferManager();
      m_vertexBuffer                     = pBuffers->createBuffer(BufferUsageHint_Vertices | BufferUsageHint_Dynamic);
      m_indexBuffer                      = pBuffers->createBuffer(BufferUsageHint_Indices  | BufferUsageHint_Dynamic);
      m_vertexArray                      = pBuffers->createVertexArray();

      // Setup backend capabilities flags
      io.BackendRendererUserData = nullptr;
      io.BackendRendererName     = "bfc_backend";
      // io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

      graphics::ShaderManager * pShaders = pDevice->getShaderManager();
      m_shader.load(pDevice, {{ShaderType_Vertex, g_vertexShaderSrc}, {ShaderType_Fragment, g_fragmentShaderSrc}});
      m_sampler = pDevice->getTextureManager()->createSampler();
      pDevice->getTextureManager()->setSamplerMinFilter(m_sampler, FilterMode_Linear, FilterMode_None);
      pDevice->getTextureManager()->setSamplerMagFilter(m_sampler, FilterMode_Linear, FilterMode_None);

      pBuffers->setVertexBuffer(m_vertexArray, 0, m_vertexBuffer);
      pBuffers->setIndexBuffer(m_vertexArray, m_indexBuffer, DataType_UInt32);

      VertexInputLayout layout;
      layout.setAttribute("POSITION0", {0, DataType_Float32, DataClass_Vector, 2, 1, IM_OFFSETOF(ImDrawVert, pos), sizeof(ImDrawVert)});
      layout.setAttribute("COLOUR0", {0, DataType_UInt8, DataClass_Vector, 4, 1, IM_OFFSETOF(ImDrawVert, col), sizeof(ImDrawVert), LayoutFlag_Normalize});
      layout.setAttribute("TEXCOORD0", {0, DataType_Float32, DataClass_Vector, 2, 1, IM_OFFSETOF(ImDrawVert, uv), sizeof(ImDrawVert)});
      pBuffers->setLayout(m_vertexArray, layout);

      // Build texture atlas
      unsigned char * pixels;
      int             width, height;
      io.Fonts->GetTexDataAsRGBA32(&pixels, &width,
                                   &height); // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to
                                             // be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a
                                             // GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

      graphics::TextureManager * pTextures = pDevice->getTextureManager();
      m_fontTexture                        = pTextures->createTexture(TextureType_2D);

      media::Surface fontSurface;
      fontSurface.pBuffer = pixels;
      fontSurface.size    = {width, height, 1};
      fontSurface.format  = PixelFormat_RGBAu8;
      pTextures->upload(m_fontTexture, fontSurface);
      pTextures->generateMipMaps(m_fontTexture);

      // Store our identifier
      io.Fonts->SetTexID(m_fontTexture);
      m_updateMonitors = true;

      // Listen for input events.
      m_pListener = m_events.addListener();
      m_pListener->on<events::KeyDown>([](events::KeyDown const & o) {
        ImGuiKey kc = translateKeyCode(o.code);
        if (kc != ImGuiKey_None)
          ImGui::GetIO().AddKeyEvent(kc, true);
        return false;
      });

      m_pListener->on<events::KeyUp>([](events::KeyUp const & o) {
        ImGuiKey kc = translateKeyCode(o.code);
        if (kc != ImGuiKey_None)
          ImGui::GetIO().AddKeyEvent(kc, false);
        return false;
      });

      m_pListener->on<events::MouseDown>([](events::MouseDown const & o) {
        ImGuiMouseButton mb = translateMouseCode(o.code);
        if (mb != ImGuiMouseButton_COUNT)
          ImGui::GetIO().AddMouseButtonEvent(mb, true);
        return false;
      });

      m_pListener->on<events::MouseUp>([](events::MouseUp const & o) {
        ImGuiMouseButton mb = translateMouseCode(o.code);
        if (mb != ImGuiMouseButton_COUNT)
          ImGui::GetIO().AddMouseButtonEvent(mb, false);
        return false;
      });

      m_pListener->on<events::MouseScroll>([](events::MouseScroll const & o) {
        ImGui::GetIO().AddMouseWheelEvent(o.horizontal ? o.amount : 0, o.horizontal ? 0 : o.amount);
        return false;
      });

      m_pListener->on<events::Character>([](events::Character const & o) {
        ImGui::GetIO().AddInputCharacter(o.c);
        return false;
      });
    }

    void Context::deinit() {
      ImGui::DestroyContext(ImGui::GetCurrentContext());
      m_vertexArray  = InvalidGraphicsResource;
      m_vertexBuffer = InvalidGraphicsResource;
      m_indexBuffer  = InvalidGraphicsResource;
      m_fontTexture  = InvalidGraphicsResource;
      m_shader.release();
    }

    void Context::beginFrame(Vec2 size) {
      bind();

      ImGuiIO & io = ImGui::GetIO();
      ImGuiPlatformIO & platformIO = ImGui::GetPlatformIO();

      if (m_updateMonitors) {
        platformIO.Monitors.clear();
        for (Display const & display : enumerateDisplays()) {
          ImGuiPlatformMonitor imDisplay;
          imDisplay.DpiScale = display.dpiScale;
          imDisplay.MainPos  = display.mainPos;
          imDisplay.MainSize = display.mainSize;
          imDisplay.WorkPos  = display.workPos;
          imDisplay.WorkSize = display.workSize;

          platformIO.Monitors.push_back(imDisplay);
        }
        m_updateMonitors = false;
      }

      // Setup display size every frame to accommodate for window resizing
      float dpiScale             = platformIO.Viewports[0]->DpiScale;
      io.DisplayFramebufferScale = ImVec2(1, 1);
      // Scale display size using the primary monitors DPI scale.
      io.DisplaySize             = size / dpiScale;

      // Update OS mouse position
      updateMouseData();

      // Update OS mouse cursor with the cursor requested by imgui
      ImGuiMouseCursor mouseCursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
      if (m_lastMouseCursor != mouseCursor) {
        m_lastMouseCursor = mouseCursor;
        setCursorIcon(getCursorIcon(mouseCursor));
      }

      ImGui::NewFrame();
      ImGuizmo::BeginFrame();
    }

    void Context::renderDrawData(ImDrawData * pDrawData) {
      graphics::StateManager *  pState   = m_pDevice->getStateManager();
      graphics::ShaderManager * pShaders = m_pDevice->getShaderManager();
      graphics::BufferManager * pBuffers = m_pDevice->getBufferManager();
      int fbWidth  = (int)(pDrawData->DisplaySize.x * pDrawData->FramebufferScale.x);
      int fbHeight = (int)(pDrawData->DisplaySize.y * pDrawData->FramebufferScale.y);
      if (fbWidth <= 0 || fbHeight <= 0)
        return;

      if (pDrawData->TotalVtxCount == 0 || pDrawData->TotalIdxCount == 0)
        return;

      int64_t vbBytes = pDrawData->TotalVtxCount * sizeof(ImDrawVert);
      int64_t ibBytes = pDrawData->TotalIdxCount * sizeof(uint32_t);

      pBuffers->upload(m_vertexBuffer, vbBytes, 0);
      pBuffers->upload(m_indexBuffer, ibBytes, 0);

      ImDrawVert * pVertData  = (ImDrawVert *)pBuffers->map(m_vertexBuffer, MapAccess_Write);
      uint32_t *   pIndexData = (uint32_t *)pBuffers->map(m_indexBuffer, MapAccess_Write);
      {
        int64_t indexOffset  = 0;
        int64_t vertexOffset = 0;
        for (int64_t i = 0; i < pDrawData->CmdListsCount; ++i) {
          ImDrawList * pCmd = pDrawData->CmdLists[i];
          memcpy(pVertData + vertexOffset, pCmd->VtxBuffer.Data, pCmd->VtxBuffer.Size * sizeof(ImDrawVert));

          for (int64_t j = 0; j < pCmd->IdxBuffer.Size; ++j) {
            pIndexData[indexOffset + j] = (uint32_t)(pCmd->IdxBuffer[(int)j] + vertexOffset);
          }

          indexOffset += pCmd->IdxBuffer.Size;
          vertexOffset += pCmd->VtxBuffer.Size;
        }
      }
      pBuffers->unmap(m_vertexBuffer);
      pBuffers->unmap(m_indexBuffer);

      pState->setViewport({0, 0}, {fbWidth, fbHeight});

      float L = pDrawData->DisplayPos.x;
      float R = pDrawData->DisplayPos.x + pDrawData->DisplaySize.x;
      float T = pDrawData->DisplayPos.y;
      float B = pDrawData->DisplayPos.y + pDrawData->DisplaySize.y;

      Mat4 orthoProj = {
        {2.0f / (R - L), 0.0f, 0.0f, (R + L) / (L - R)},
        {0.0f, 2.0f / (T - B), 0.0f, (T + B) / (B - T)},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
      };

      m_pDevice->bindProgram(m_shader.getResource());
      m_shader.setUniform("ProjMtx", orthoProj);
      m_shader.setTextureBinding("Texture", 0);

      m_pDevice->bindSampler(m_sampler, 0);
      m_pDevice->bindVertexArray(m_vertexArray);

      pState->setFeatureEnabled(GraphicsState_Blend, true);
      pState->setFeatureEnabled(GraphicsState_DepthTest, false);
      pState->setFeatureEnabled(GraphicsState_ScissorTest, true);

      ImVec2 clipOff   = pDrawData->DisplayPos;       // (0,0) unless using multi-viewports
      ImVec2 clipScale = pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

      int64_t indexOffset  = 0;
      int64_t vertexOffset = 0;

      for (int n = 0; n < pDrawData->CmdListsCount; ++n) {
        const ImDrawList * pCmdList = pDrawData->CmdLists[n];

        for (int cmdIndex = 0; cmdIndex < pCmdList->CmdBuffer.Size; ++cmdIndex) {
          const ImDrawCmd * pCmd = &pCmdList->CmdBuffer[cmdIndex];
          if (pCmd->UserCallback != NULL) {
            pCmd->UserCallback(pCmdList, pCmd);
          } else {
            // Project scissor/clipping rectangles into framebuffer space
            ImVec2 clipMin((pCmd->ClipRect.x - clipOff.x) * clipScale.x, (pCmd->ClipRect.y - clipOff.y) * clipScale.y);
            ImVec2 clipMax((pCmd->ClipRect.z - clipOff.x) * clipScale.x, (pCmd->ClipRect.w - clipOff.y) * clipScale.y);
            if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
              continue;

            // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
            pState->setScissor({(int)clipMin.x, (int)((float)fbHeight - clipMax.y)}, {(int)(clipMax.x - clipMin.x), (int)(clipMax.y - clipMin.y)});

            m_pDevice->bindTexture((GraphicsResource)pCmd->GetTexID(), 0);
            m_pDevice->drawIndexed(pCmd->ElemCount, indexOffset + pCmd->IdxOffset, PrimitiveType_Triangle);
          }
        }
        indexOffset += pCmdList->IdxBuffer.Size;
      }

      m_pDevice->bindSampler(InvalidGraphicsResource, 0);
      pState->setFeatureEnabled(GraphicsState_ScissorTest, false);
    }

    void Context::bind() {
      ImGui::SetCurrentContext(m_pImContext);
    }

    Context * Context::Current() {
      return (Context *)ImGui::GetIO().UserData;
    }

    void Context::updateMouseData() {
      ImGuiIO & io = ImGui::GetIO();

      bfc::Vec2i mousePos = bfc::getCursorPosition();

      platform::Window * pForeground = platform::Window::getForegroundWindow();
      if (pForeground != nullptr) {
        ImGuiViewport * pViewport = ImGui::FindViewportByPlatformHandle(pForeground);

        if (io.WantSetMousePos) {
          Vec2i mousePos  = glm::ceil(Vec2(io.MousePos));
          Vec2i screenPos = pForeground->clientToScreen(mousePos);
          setCursorPosition(screenPos);
        }

        if (!io.WantSetMousePos) {
          Vec2i clientPos = getCursorPosition();

          if ((io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0) {
            clientPos = pForeground->screenToClient(clientPos);
          }

          io.AddMousePosEvent((float)clientPos.x, (float)clientPos.y);
        }
      }

      platform::Window * pHovered          = platform::Window::findWindow(mousePos);
      ImGuiID            hoveredViewportID = 0;
      if (pHovered != nullptr) {
        if (ImGuiViewport * viewport = ImGui::FindViewportByPlatformHandle(pHovered)) {
          hoveredViewportID = viewport->ID;
        }
      }

      io.AddMouseViewportEvent(hoveredViewportID);
    }

    GraphicsDevice * Context::getDevice() const {
      return m_pDevice;
    }

    void Context::requestUpdateMonitors() {
      m_updateMonitors = true;
    }

    TextureAtlas * Context::getTextureAtlas() {
      return m_pAtlas;
    }

    CursorIcon getCursorIcon(ImGuiMouseCursor cursor) {
      switch (cursor) {
      case ImGuiMouseCursor_Arrow: return CursorIcon_Arrow;
      case ImGuiMouseCursor_TextInput: return CursorIcon_TextInput;
      case ImGuiMouseCursor_ResizeAll: return CursorIcon_ResizeAll;
      case ImGuiMouseCursor_ResizeNS: return CursorIcon_ResizeNS;
      case ImGuiMouseCursor_ResizeEW: return CursorIcon_ResizeEW;
      case ImGuiMouseCursor_ResizeNESW: return CursorIcon_ResizeNESW;
      case ImGuiMouseCursor_ResizeNWSE: return CursorIcon_ResizeNWSE;
      case ImGuiMouseCursor_Hand: return CursorIcon_Hand;
      case ImGuiMouseCursor_NotAllowed: return CursorIcon_NotAllowed;
      }

      return CursorIcon_None;
    }

    ImGuiKey translateKeyCode(KeyCode key) {
      switch (key) {
      case KeyCode_Tab: return ImGuiKey_Tab;
      case KeyCode_LeftArrow: return ImGuiKey_LeftArrow;
      case KeyCode_RightArrow: return ImGuiKey_RightArrow;
      case KeyCode_UpArrow: return ImGuiKey_UpArrow;
      case KeyCode_DownArrow: return ImGuiKey_DownArrow;
      case KeyCode_Home: return ImGuiKey_Home;
      case KeyCode_End: return ImGuiKey_End;
      case KeyCode_Insert: return ImGuiKey_Insert;
      case KeyCode_Delete: return ImGuiKey_Delete;
      case KeyCode_Backspace: return ImGuiKey_Backspace;
      case KeyCode_Space: return ImGuiKey_Space;
      case KeyCode_Enter: return ImGuiKey_Enter;
      case KeyCode_Escape: return ImGuiKey_Escape;
      case KeyCode_Control: return ImGuiKey_LeftCtrl;
      case KeyCode_Shift: return ImGuiKey_LeftShift;
      case KeyCode_Alt: return ImGuiKey_LeftAlt;
      case KeyCode_Super: return ImGuiKey_LeftSuper;
      case KeyCode_0: return ImGuiKey_0;
      case KeyCode_1: return ImGuiKey_1;
      case KeyCode_2: return ImGuiKey_2;
      case KeyCode_3: return ImGuiKey_3;
      case KeyCode_4: return ImGuiKey_4;
      case KeyCode_5: return ImGuiKey_5;
      case KeyCode_6: return ImGuiKey_6;
      case KeyCode_7: return ImGuiKey_7;
      case KeyCode_8: return ImGuiKey_8;
      case KeyCode_9: return ImGuiKey_9;
      case KeyCode_A: return ImGuiKey_A;
      case KeyCode_B: return ImGuiKey_B;
      case KeyCode_C: return ImGuiKey_C;
      case KeyCode_D: return ImGuiKey_D;
      case KeyCode_E: return ImGuiKey_E;
      case KeyCode_F: return ImGuiKey_F;
      case KeyCode_G: return ImGuiKey_G;
      case KeyCode_H: return ImGuiKey_H;
      case KeyCode_I: return ImGuiKey_I;
      case KeyCode_J: return ImGuiKey_J;
      case KeyCode_K: return ImGuiKey_K;
      case KeyCode_L: return ImGuiKey_L;
      case KeyCode_M: return ImGuiKey_M;
      case KeyCode_N: return ImGuiKey_N;
      case KeyCode_O: return ImGuiKey_O;
      case KeyCode_P: return ImGuiKey_P;
      case KeyCode_Q: return ImGuiKey_Q;
      case KeyCode_R: return ImGuiKey_R;
      case KeyCode_S: return ImGuiKey_S;
      case KeyCode_T: return ImGuiKey_T;
      case KeyCode_U: return ImGuiKey_U;
      case KeyCode_V: return ImGuiKey_V;
      case KeyCode_W: return ImGuiKey_W;
      case KeyCode_X: return ImGuiKey_X;
      case KeyCode_Y: return ImGuiKey_Y;
      case KeyCode_Z: return ImGuiKey_Z;
      case KeyCode_F1: return ImGuiKey_F1;
      case KeyCode_F2: return ImGuiKey_F2;
      case KeyCode_F3: return ImGuiKey_F3;
      case KeyCode_F4: return ImGuiKey_F4;
      case KeyCode_F5: return ImGuiKey_F5;
      case KeyCode_F6: return ImGuiKey_F6;
      case KeyCode_F7: return ImGuiKey_F7;
      case KeyCode_F8: return ImGuiKey_F8;
      case KeyCode_F9: return ImGuiKey_F9;
      case KeyCode_F10: return ImGuiKey_F10;
      case KeyCode_F11: return ImGuiKey_F11;
      case KeyCode_F12: return ImGuiKey_F12;
      case KeyCode_Apostrophe: return ImGuiKey_Apostrophe;
      case KeyCode_Comma: return ImGuiKey_Comma;
      case KeyCode_Minus: return ImGuiKey_Minus;
      case KeyCode_Period: return ImGuiKey_Period;
      case KeyCode_Slash: return ImGuiKey_Slash;
      case KeyCode_Semicolon: return ImGuiKey_Semicolon;
      case KeyCode_Equals: return ImGuiKey_Equal;
      case KeyCode_LeftBracket: return ImGuiKey_LeftBracket;
      case KeyCode_Backslash: return ImGuiKey_Backslash;
      case KeyCode_RightBracket: return ImGuiKey_RightBracket;
      case KeyCode_Grave: return ImGuiKey_GraveAccent;
      case KeyCode_CapsLock: return ImGuiKey_CapsLock;
      case KeyCode_ScrollLock: return ImGuiKey_ScrollLock;
      case KeyCode_NumLock: return ImGuiKey_NumLock;
      case KeyCode_PrintScreen: return ImGuiKey_PrintScreen;
      case KeyCode_Pause: return ImGuiKey_Pause;
      case KeyCode_Keypad0: return ImGuiKey_Keypad0;
      case KeyCode_Keypad1: return ImGuiKey_Keypad1;
      case KeyCode_Keypad2: return ImGuiKey_Keypad2;
      case KeyCode_Keypad3: return ImGuiKey_Keypad3;
      case KeyCode_Keypad4: return ImGuiKey_Keypad4;
      case KeyCode_Keypad5: return ImGuiKey_Keypad5;
      case KeyCode_Keypad6: return ImGuiKey_Keypad6;
      case KeyCode_Keypad7: return ImGuiKey_Keypad7;
      case KeyCode_Keypad8: return ImGuiKey_Keypad8;
      case KeyCode_Keypad9: return ImGuiKey_Keypad9;
      case KeyCode_KeypadDecimal: return ImGuiKey_KeypadDecimal;
      case KeyCode_KeypadDivide: return ImGuiKey_KeypadDivide;
      case KeyCode_KeypadMultiply: return ImGuiKey_KeypadMultiply;
      case KeyCode_KeypadSubtract: return ImGuiKey_KeypadSubtract;
      case KeyCode_KeypadAdd: return ImGuiKey_KeypadAdd;
      case KeyCode_KeypadEnter: return ImGuiKey_KeypadEnter;
      case KeyCode_KeypadEqual: return ImGuiKey_KeypadEqual;
      case KeyCode_ModCtrl: return ImGuiKey_ModCtrl;
      case KeyCode_ModShift: return ImGuiKey_ModShift;
      case KeyCode_ModAlt: return ImGuiKey_ModAlt;
      case KeyCode_ModSuper: return ImGuiKey_ModSuper;
      }

      return ImGuiKey_None;
    }

    ImGuiMouseButton translateMouseCode(MouseButton bt) {
      switch (bt) {
      case MouseButton_Left: return ImGuiMouseButton_Left;
      case MouseButton_Right: return ImGuiMouseButton_Right;
      case MouseButton_Middle: return ImGuiMouseButton_Middle;
      }
      return ImGuiMouseButton_COUNT;
    }
  } // namespace ui
} // namespace bfc
