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
    GBuffer(graphics::CommandList * pCmdList, Vec2i const & size);

    /// Set the resolution of the gbuffer.
    bool resize(graphics::CommandList * pCmdList, Vec2i const & size);

    inline graphics::TextureRef const & getBaseColour() const {
      return m_target[GBufferTarget_BaseColour];
    }

    inline graphics::TextureRef const & getAmbientColour() const {
      return m_target[GBufferTarget_AmbientColour];
    }

    inline graphics::TextureRef const & getPosition() const {
      return m_target[GBufferTarget_Position];
    }

    inline graphics::TextureRef const & getNormal() const {
      return m_target[GBufferTarget_Normal];
    }

    inline graphics::TextureRef const & getRMA() const {
      return m_target[GBufferTarget_RMA];
    }

    inline graphics::TextureRef const & getDepthTarget() const {
      return m_depthTarget;
    }

    inline graphics::RenderTargetRef const & getRenderTarget() const {
      return m_renderTarget;
    }

    graphics::TextureRef const & operator[](int64_t i) const {
      return m_target[i];
    }

    graphics::TextureRef const * begin() const {
      return m_target;
    }

    graphics::TextureRef const * end() const {
      return m_target + GBufferTarget_Count;
    }

  private:
    Vec2i   m_resolution = { 0, 0 };
    graphics::TextureRef m_target[GBufferTarget_Count];
    graphics::TextureRef m_depthTarget;

    graphics::RenderTargetRef m_renderTarget;
  };
} // namespace bfc
