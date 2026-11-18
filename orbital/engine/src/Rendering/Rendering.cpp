#include "Rendering.h"
#include "Application.h"
#include "Viewport/Viewport.h"

#include "render/GraphicsDevice.h"
#include "platform/Window.h"
#include "Levels/LevelManager.h"

using namespace engine;
using namespace bfc;

namespace {
  Setting<String>       g_api;
  Ref<GraphicsDevice>   g_pDevice       = nullptr;
  Ref<platform::Window> g_pWindow       = nullptr;
  Ref<EventListener>    g_pListener     = nullptr;
  Ref<Viewport>         g_pMainViewport = nullptr;

  Vector<Ref<IRenderingPlugin>>    g_plugins;
  Vector<Ref<IRenderingExtension>> g_extensions;
  uint64_t                         g_lastFrameFence = 0;
  Vector<WeakRef<Renderer>>        g_renderers;
} // namespace

namespace engine {
  namespace rendering {
    using namespace bfc::graphics;

    GraphicsDevice *getDevice() {
      return g_pDevice.get();
    }

    platform::Window *getMainWindow() {
      return g_pWindow.get();
    }

    bool init(Application *pApp) {
      graphicsDevice_registerOpenGL();

      g_pWindow = NewRef<platform::Window>(pApp);
      g_pWindow->setTitle(pApp->getName());
      g_pWindow->show();

      g_pListener = g_pWindow->getEvents()->addListener();
      g_pListener->on<bfc::events::CloseWindow>([=](bfc::events::CloseWindow const & e) {
        pApp->exit();
        return true;
      });

      g_api = pApp->addSetting<String>("rendering/api", "OpenGL");

      g_pDevice = createGraphicsDevice(g_api.get());
      if (g_pDevice == nullptr) {
        return false;
      }

      if (!g_pDevice->init(g_pWindow.get())) {
        return false;
      }

      return true;
    }

    void shutdown() {
      g_pDevice = nullptr;
      g_pWindow = nullptr;
    }

    void loop() {
      g_renderers.eraseIf([](const auto & o) { return o.expired(); });

      auto pCmdList = g_pDevice->createCommandList();
      pCmdList->setDebugName("rendering::loop MainViewport");

      pCmdList->bindRenderTarget(g_pDevice->getDefaultRenderTarget());
      pCmdList->swap();
      pCmdList->clear({0, 0, 0, 1});
      for (auto & pPlugin : g_plugins)
        pPlugin->onFrame(pCmdList.get(), g_pWindow.get(), g_pDevice->getDefaultRenderTarget());
      pCmdList->bindRenderTarget(g_pDevice->getDefaultRenderTarget());

      // m_pMainViewport->setSize(pCmdList.get(), m_pWindow->getSize());
      // m_pMainViewport->render(pCmdList.get(), m_pDevice->getDefaultRenderTarget());
      uint64_t thisFrameFence = g_pDevice->submit(std::move(pCmdList));

      g_pDevice->wait(g_lastFrameFence);
      g_lastFrameFence = thisFrameFence;
    }

    void registerPlugin(Ref<IRenderingPlugin> pPlugin) {
      g_plugins.pushBack(pPlugin);
    }

    bool unregisterExtension(Ref<IRenderingPlugin> pPlugin) {
      return g_plugins.eraseValue(pPlugin);
    }

    void registerExtension(Ref<IRenderingExtension> const &pExtension) {
      g_extensions.pushBack(pExtension);

      for (WeakRef<Renderer> const & pRenderer : g_renderers)
        if (auto pLocked = pRenderer.lock())
          pExtension->apply(pLocked.get());
    }

    bool unregisterExtension(Ref<IRenderingExtension> const &pExtension) {
      return g_extensions.eraseValue(pExtension);
    }

    void registerRenderer(Ref<Renderer> const &pRenderer) {
      for (Ref<IRenderingExtension> const & pExtension : g_extensions)
        pExtension->apply(pRenderer.get());

      g_renderers.pushBack(pRenderer);
    }

    Ref<Viewport> getMainViewport() {
      return g_pMainViewport;
    }

    graphics::RenderTargetRef getDefaultRenderTarget() {
      return getDevice()->getDefaultRenderTarget();
    }

    std::unique_ptr<graphics::CommandList> createCommandList() {
      return getDevice()->createCommandList();
    }

    graphics::BufferRef createBuffer(BufferUsageHint usageHint) {
      return getDevice()->createBuffer();
    }

    graphics::VertexArrayRef createVertexArray() {
      return getDevice()->createVertexArray();
    }

    graphics::ProgramRef createProgram() {
      return getDevice()->createProgram();
    }

    graphics::TextureRef createTexture(TextureType type) {
      return getDevice()->createTexture(type);
    }

    graphics::TextureDownloadRef createTextureDownload() {
      return getDevice()->createTextureDownload();
    }

    graphics::SamplerRef createSampler() {
      return getDevice()->createSampler();
    }

    graphics::RenderTargetRef createRenderTarget(RenderTargetType type) {
      return getDevice()->createRenderTarget(type);
    }

    graphics::StateManager* getStateManager() {
      return getDevice()->getStateManager();
    }

    std::shared_future<bool> compile(graphics::ProgramRef pProgram) {
      return getDevice()->compile(pProgram);
    }

    uint64_t submit(std::unique_ptr<graphics::CommandList> &&pCommandList) {
      return getDevice()->submit(std::move(pCommandList));
    }

    bool wait(uint64_t handle, std::optional<Timestamp> const & timeout) {
      return getDevice()->wait(handle, timeout);
    }
  } // namespace rendering
}
