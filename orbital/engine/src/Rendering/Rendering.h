#pragma once

#include "../../../../vendor/imgui/imgui_internal.h"
#include "Subsystem.h"
#include "util/Settings.h"
#include "render/GraphicsDevice.h"

namespace bfc {
  class GraphicsDevice;
  class ShaderPool;

  namespace platform {
    class Window;
  }
} // namespace bfc

namespace engine {
  class Viewport;
  class Renderer;
  class Rendering;
  class Renderer;

  class IRenderingExtension {
  public:
    virtual void apply(Renderer * pRenderer) = 0;
  };

  class IRenderingPlugin {
  public:
    virtual void onFrame(bfc::graphics::CommandList * pCmdList, bfc::platform::Window * pWindow,
                         bfc::graphics::RenderTargetRef renderTarget) {
      BFC_UNUSED(pCmdList, pWindow, renderTarget);
    }
  };

  class Rendering : public Subsystem {
  public:
    Rendering();

    bfc::GraphicsDevice * getDevice() const;

    bfc::platform::Window * getMainWindow() const;

    virtual bool init(Application * pApp) override;

    virtual void shutdown() override;

    virtual void loop(Application * pApp) override;

    /// Register a plugin.
    void registerPlugin(bfc::Ref<IRenderingPlugin> pPlugin);

    /// Unregister a plugin.
    bool unregisterExtension(bfc::Ref<IRenderingPlugin> pPlugin);

    /// Register a renderer extension
    void registerExtension(bfc::Ref<IRenderingExtension> const & pExtension);

    /// Unregister a renderer extension
    bool unregisterExtension(bfc::Ref<IRenderingExtension> const & pExtension);

    /// Register a renderer with the rendering system.
    void registerRenderer(bfc::Ref<Renderer> const & pRenderer);

    /// Get the viewport rendered to the main window.
    bfc::Ref<Viewport> getMainViewport();

  private:
    bfc::Ref<bfc::GraphicsDevice>   m_pDevice       = nullptr;
    bfc::Ref<bfc::platform::Window> m_pWindow       = nullptr;
    bfc::Ref<bfc::EventListener>    m_pListener     = nullptr;
    bfc::Ref<Viewport>              m_pMainViewport = nullptr;
    bfc::Setting<bfc::String>       m_api;

    bfc::Vector<bfc::WeakRef<Renderer>>        m_renderers;
    bfc::Vector<bfc::Ref<IRenderingPlugin>>    m_plugins;
    bfc::Vector<bfc::Ref<IRenderingExtension>> m_extensions;

    uint64_t m_lastFrameFence = 0;
  };
} // namespace engine
