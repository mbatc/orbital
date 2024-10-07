#pragma once

#include "GraphicsDevice.h"
#include "Texture.h"

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
    GBuffer(GraphicsDevice * pDevice, Vec2i const & size);

    /// Set the resolution of the gbuffer.
    bool resize(Vec2i const & size);

    inline Texture const & getBaseColour() const {
      return m_target[GBufferTarget_BaseColour];
    }

    inline Texture const & getAmbientColour() const {
      return m_target[GBufferTarget_AmbientColour];
    }

    inline Texture const & getPosition() const {
      return m_target[GBufferTarget_Position];
    }

    inline Texture const & getNormal() const {
      return m_target[GBufferTarget_Normal];
    }

    inline Texture const& getRMA() const {
      return m_target[GBufferTarget_RMA];
    }

    inline Texture const & getDepthTarget() const {
      return m_depthTarget;
    }

    inline ManagedGraphicsResource const & getRenderTarget() const {
      return m_renderTarget;
    }

    Texture const& operator[](int64_t i) const {
      return m_target[i];
    }

    Texture const* begin() const {
      return m_target;
    }

    Texture const * end() const {
      return m_target + GBufferTarget_Count;
    }

  private:
    Vec2i   m_resolution = { 0, 0 };
    Texture m_target[GBufferTarget_Count];
    Texture m_depthTarget;

    ManagedGraphicsResource m_renderTarget;

    GraphicsDevice * m_pDevice = nullptr;
  };
} // namespace bfc
