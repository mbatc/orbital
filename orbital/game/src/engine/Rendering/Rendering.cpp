#include "Rendering.h"
#include "Application.h"
#include "Viewport/Viewport.h"

#include "render/GraphicsDevice.h"
#include "render/Shader.h"
#include "platform/Window.h"
#include "Levels/LevelManager.h"

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

    m_pListener = m_pWindow->getEvents()->addListener();
    m_pListener->on<bfc::events::CloseWindow>([=](bfc::events::CloseWindow const & e) {
      pApp->exit();
      return true;
    });

    m_api = pApp->addSetting<String>("rendering/api", "OpenGL");

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

  void Rendering::loop(Application * pApp)
  {
    auto pCmdList = m_pDevice->createCommandList();

    BFC_UNUSED(pApp);
    pCmdList->bindRenderTarget(InvalidGraphicsResource);
    pCmdList->swap();
    pCmdList->clear({0, 0, 0, 1});

    m_pMainViewport->setSize(pCmdList.get(), m_pWindow->getSize());
    m_pMainViewport->render(pCmdList.get(), InvalidGraphicsResource);
    uint64_t thisFrameFence = m_pDevice->submit(std::move(pCmdList));

    m_pDevice->wait(m_lastFrameFence);
    m_lastFrameFence = thisFrameFence;
    {
      events::OnRenderViewport e;
      e.pViewport = m_pMainViewport.get();
      e.pDevice   = m_pDevice.get();
      e.isMainViewport = true;
      pApp->broadcast(e);
    }
  }

  void Rendering::setMainViewport(bfc::Ref<Viewport> const & pViewport) {
    if (m_pMainViewport != nullptr) {
      m_pMainViewport->getEvents()->stopListening(m_pWindow->getEvents());
    }

    m_pMainViewport = pViewport;
    m_pMainViewport->getEvents()->listenTo(m_pWindow->getEvents());
  }

  bfc::Ref<Viewport> Rendering::getMainViewport() {
    return m_pMainViewport;
  }
}
