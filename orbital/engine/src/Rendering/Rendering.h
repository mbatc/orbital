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
  using namespace bfc;

  class Viewport;
  class Renderer;

  class IRenderingExtension {
  public:
    virtual void apply(Renderer * pRenderer) = 0;
  };

  class IRenderingPlugin {
  public:
    virtual void onFrame(graphics::CommandList * pCmdList, platform::Window * pWindow,
                         graphics::RenderTargetRef renderTarget) {
      BFC_UNUSED(pCmdList, pWindow, renderTarget);
    }
  };

  namespace rendering {
    bool init(Application * pApp);
    void loop();
    void shutdown();

    GraphicsDevice* getDevice();

    platform::Window* getMainWindow();

    /// Register a plugin.
    void registerPlugin(Ref<IRenderingPlugin> pPlugin);

    /// Unregister a plugin.
    bool unregisterExtension(Ref<IRenderingPlugin> pPlugin);

    /// Register a renderer extension
    void registerExtension(Ref<IRenderingExtension> const & pExtension);

    /// Unregister a renderer extension
    bool unregisterExtension(Ref<IRenderingExtension> const & pExtension);

    /// Register a renderer with the rendering system.
    void registerRenderer(Ref<Renderer> const & pRenderer);

    /// Get the viewport rendered to the main window.
    Ref<Viewport> getMainViewport();

    // Graphics Device API

    /// Get the default render target that was created during initialisation.
    /// This render target is for the window passed to the Init() function.
    /// @returns A reference to the default render target.
    graphics::RenderTargetRef getDefaultRenderTarget();

    /// Get the state manager
    std::unique_ptr<graphics::CommandList> createCommandList();

    /// Create a new buffer resource.
    /// @param usageHint A bitfield of usage flags indicating how the buffer is intended to be used.
    /// @returns A GraphicsResource that refers to the new buffer.
    graphics::BufferRef createBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown);

    /// Create a new vertex array.
    graphics::VertexArrayRef createVertexArray();

    /// Create a new shader.
    /// Create a new shader program.
    graphics::ProgramRef createProgram();

    /// Create a new texture resource.
    /// @param type The type of texture to create.
    graphics::TextureRef createTexture(TextureType type);

    /// Create a new texture download interface.
    graphics::TextureDownloadRef createTextureDownload();

    /// Create a new sampler resource.
    graphics::SamplerRef createSampler();

    /// Create a new render target resource.
    /// @param type The type of render target to create.
    graphics::RenderTargetRef createRenderTarget(RenderTargetType type);

    /// Get the state manager
    graphics::StateManager * getStateManager();

    /// Compile a shader program.
    /// @returns a future that will resolve to the result of the compilation job.
    std::shared_future<bool> compile(graphics::ProgramRef pProgram);

    /// Submit a command list to be executed.
    uint64_t submit(std::unique_ptr<graphics::CommandList> && pCommandList);

    /// Wait for a command list to complete execution.
    bool wait(uint64_t handle, std::optional<Timestamp> const & timeout = std::nullopt);
  }
} // namespace engine
