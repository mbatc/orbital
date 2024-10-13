#pragma once

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
  class Rendering : public Subsystem {
  public:
    Rendering();

    bfc::GraphicsDevice * getDevice() const;

    bfc::platform::Window* getMainWindow() const;

    virtual bool init(Application * pApp) override;

    virtual void shutdown() override;

    virtual void loop() override;

  private:
    bfc::Ref<bfc::GraphicsDevice> m_pDevice = nullptr;
    bfc::Ref<bfc::platform::Window> m_pWindow = nullptr;
    bfc::Ref<bfc::EventListener>    m_pListener = nullptr;
    bfc::Setting<bfc::String> m_api;
  };
}
