#include "Rendering.h"
#include "Application.h"

#include "render/GraphicsDevice.h"
#include "render/Shader.h"
#include "platform/Window.h"

using namespace bfc;
using namespace bfc::platform;

namespace engine {
  Rendering::Rendering() 
    : Subsystem(TypeID<Rendering>(), "Rendering") {
    graphicsDevice_registerOpenGL();
  }

  GraphicsDevice* Rendering::getDevice() const
  {
    return m_pDevice.get();
  }

  Window* Rendering::getMainWindow() const
  {
    return m_pWindow.get();
  }

  bool Rendering::init(Application *pApp)
  {
    m_pWindow = NewRef<Window>(pApp);
    m_pWindow->setTitle(pApp->getName());
    m_pWindow->show();

    m_api = pApp->addSetting<String>("Rendering/api", "OpenGL");

    m_pDevice = createGraphicsDevice(m_api.get());
    if (m_pDevice == nullptr) {
      return false;
    }

    if (!m_pDevice->init(m_pWindow.get())) {
      return false;
    }

    return true;
  }

  void Rendering::shutdown()
  {
    m_pDevice = nullptr;
    m_pWindow = nullptr;
  }

  void Rendering::loop()
  {
    m_pDevice->bindRenderTarget(InvalidGraphicsResource);
    m_pDevice->swap();
    m_pDevice->clear({ 0, 0, 0, 1 });
  }
}
