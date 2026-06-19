#pragma once

#include "GraphicsDevice.h"

namespace bfc {
  enum GBufferTarget {
    GBufferTarget_BaseColour,
    GBufferTarget_AmbientColour,
    GBufferTarget_Position,
    GBufferTarget_Normal,
    GBufferTarget_RMA,
    GBufferTarget_Count
  };

  /// Simple container class that creates a HDR GBuffer.
  class BFC_API GBuffer {
  public:
    struct Textures {
      Vec2i                resolution = {0, 0};
      graphics::TextureRef target[GBufferTarget_Count];
      graphics::TextureRef depthTarget;
    };

    /// Create a gbuffer render target
    /// @param pCmdList Command list used to create the texture resources.
    /// @param textures Textures to use for the targets. If any are new textures will be created.
    GBuffer(graphics::CommandList * pCmdList, Textures const & textures = {});

    /// Create a gbuffer render target
    /// @param pCmdList Command list used to create the texture resources.
    /// @param resolution Size of the render target textures.
    GBuffer(graphics::CommandList * pCmdList, Vec2i const & resolution);

    /// Set the resolution of the gbuffer.
    bool resize(graphics::CommandList * pCmdList, Vec2i const & size);

    inline Textures const& getTextures() const {
      return m_textures;
    }

    inline Vec2i const & getResolution() const {
      return m_textures.resolution;
    }

    inline graphics::TextureRef const & getBaseColour() const {
      return m_textures.target[GBufferTarget_BaseColour];
    }

    inline graphics::TextureRef const & getAmbientColour() const {
      return m_textures.target[GBufferTarget_AmbientColour];
    }

    inline graphics::TextureRef const & getPosition() const {
      return m_textures.target[GBufferTarget_Position];
    }

    inline graphics::TextureRef const & getNormal() const {
      return m_textures.target[GBufferTarget_Normal];
    }

    inline graphics::TextureRef const & getRMA() const {
      return m_textures.target[GBufferTarget_RMA];
    }

    inline graphics::TextureRef const & getDepthTarget() const {
      return m_textures.depthTarget;
    }

    inline graphics::RenderTargetRef const & getRenderTarget() const {
      return m_renderTarget;
    }

    graphics::TextureRef const & operator[](int64_t i) const {
      return m_textures.target[i];
    }

    graphics::TextureRef const * begin() const {
      return m_textures.target;
    }

    graphics::TextureRef const * end() const {
      return m_textures.target + GBufferTarget_Count;
    }

  private:
    Textures                  m_textures;
    graphics::RenderTargetRef m_renderTarget;
  };
} // namespace bfc
