#pragma once

#include "Texture.h"

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
    Texture baseColour;
    Texture ambientColour;
    Texture position;
    Texture normal;
    Texture rma;
    Texture sceneDepth;
  };

  class BFC_API PostProcessParams {
  public:
    GraphicsResource   target;           ///< Target to render the effect to
    Texture            sceneColour;      ///< Scene colour texture
    PostProcessInput * pInput = nullptr; ///< Shared post-process inputs

    /// Bind this passes inputs
    void bindInputs(GraphicsDevice * pDevice) const;
    /// Bind this passes target
    void bindTarget(GraphicsDevice * pDevice) const;
  };

  class BFC_API PostProcessingStack {
  public:
    PostProcessingStack() = default;
    PostProcessingStack(PostProcessingStack && o);
    PostProcessingStack(PostProcessingStack const & o) = delete;

    Texture          sceneColour; ///< Scene colour
    PostProcessInput inputs; ///< Auxiliary inputs
    GraphicsResource target; ///< Where to render the final colour to.

    /// Add a pass to the post-processing stack
    void addPass(std::function<void(GraphicsDevice *, PostProcessParams const &)> callback);

    /// Execute all passes added to the stack.
    void execute(GraphicsDevice * pDevice, Vec2i size);

    /// Reset the post-processing stack.
    void reset();

  private:
    struct Pass {
      std::function<void(GraphicsDevice *, PostProcessParams const &)> callback;
      PostProcessParams params;
    };
    Vector<Pass> m_passes;

    // Ping-pong buffer for processed colour
    Vec2i                   m_intermediateSize;
    int64_t                 m_currentTarget = 0;
    Texture                 m_intermediateColour[2];
    ManagedGraphicsResource m_intermediateTarget[2];
  };
}
