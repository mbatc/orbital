#pragma once

#include "GraphicsDevice.h"

namespace bfc {
  class Renderer;

  enum PostProcessInputBindPoint {
    PostProcessInputBindPoint_SceneColour,
    PostProcessInputBindPoint_SceneDepth,
    PostProcessInputBindPoint_BaseColour,
    PostProcessInputBindPoint_AmbientColour,
    PostProcessInputBindPoint_Position,
    PostProcessInputBindPoint_Normal,
    PostProcessInputBindPoint_RMA,
    PostProcessInputBindPoint_Count,
  };

  /// Post process input textures.
  /// SceneColour is omitted here as the texture used changes between passes.
  struct BFC_API PostProcessInput {
    graphics::TextureRef baseColour;
    graphics::TextureRef ambientColour;
    graphics::TextureRef position;
    graphics::TextureRef normal;
    graphics::TextureRef rma;
    graphics::TextureRef sceneDepth;
  };

  class BFC_API PostProcessParams {
  public:
    graphics::RenderTargetRef target;           ///< Target to render the effect to
    graphics::TextureRef      sceneColour;      ///< Scene colour texture
    PostProcessInput *        pInput = nullptr; ///< Shared post-process inputs

    /// Bind this passes inputs
    void bindInputs(graphics::CommandList * pCmdList) const;
    /// Bind this passes target
    void bindTarget(graphics::CommandList * pCmdList) const;
  };

  class BFC_API PostProcessingStack {
  public:
    PostProcessingStack() = default;
    PostProcessingStack(PostProcessingStack && o);
    PostProcessingStack(PostProcessingStack const & o) = delete;

    graphics::TextureRef      sceneColour; ///< Scene colour
    PostProcessInput          inputs;      ///< Auxiliary inputs
    graphics::RenderTargetRef target;      ///< Where to render the final colour to.

    /// Add a pass to the post-processing stack
    void addPass(std::function<void(graphics::CommandList *, PostProcessParams const &)> callback);

    /// Execute all passes added to the stack.
    void execute(graphics::CommandList * pDevice, Vec2i size);

    /// Reset the post-processing stack.
    void reset();

  private:
    struct Pass {
      std::function<void(graphics::CommandList *, PostProcessParams const &)> callback;
      PostProcessParams                                                params;
    };
    Vector<Pass> m_passes;

    // Ping-pong buffer for processed colour
    Vec2i                     m_intermediateSize;
    int64_t                   m_currentTarget = 0;
    graphics::TextureRef      m_intermediateColour[2];
    graphics::RenderTargetRef m_intermediateTarget[2];
  };
} // namespace bfc
