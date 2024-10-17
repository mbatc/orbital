#pragma once

#include "../../../../../vendor/imgui/imgui_internal.h"
#include "Subsystem.h"
#include "util/Settings.h"

namespace bfc {
  class GraphicsDevice;
  class ShaderPool;

  namespace platform {
    class Window;
  }
}

namespace engine {
  class Viewport;
  class Rendering : public Subsystem {
  public:
    Rendering();

    bfc::GraphicsDevice * getDevice() const;

    bfc::platform::Window* getMainWindow() const;

    virtual bool init(Application * pApp) override;

    virtual void shutdown() override;

    virtual void loop(Application * pApp) override;

    /// Set the viewport rendered to the main window.
    void setMainViewport(bfc::Ref<Viewport> const & pViewport);

    /// Get the viewport rendered to the main window.
    bfc::Ref<Viewport> getMainViewport();

  private:
    bfc::Ref<bfc::GraphicsDevice>   m_pDevice       = nullptr;
    bfc::Ref<bfc::platform::Window> m_pWindow       = nullptr;
    bfc::Ref<bfc::EventListener>    m_pListener     = nullptr;
    bfc::Ref<Viewport>              m_pMainViewport = nullptr;
    bfc::Setting<bfc::String> m_api;
  };
}
