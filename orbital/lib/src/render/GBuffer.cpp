#include "render/GBuffer.h"

namespace bfc {
  GBuffer::GBuffer(graphics::CommandList * pCmdList, Vec2i const & size)
  {
    resize(pCmdList, size);
  }

  bool GBuffer::resize(graphics::CommandList * pCmdList, Vec2i const & size) {
    if (m_resolution == size) {
      return false;
    }

    // Allocate textures
    graphics::loadTexture2D<RGBAu8>(pCmdList, &m_target[GBufferTarget_BaseColour], size);
    graphics::loadTexture2D(pCmdList, &m_target[GBufferTarget_AmbientColour], size, PixelFormat_RGBAf16);
    graphics::loadTexture2D<RGBAf32>(pCmdList, &m_target[GBufferTarget_Position], size);
    graphics::loadTexture2D<RGBAu8>(pCmdList, &m_target[GBufferTarget_Normal], size);
    graphics::loadTexture2D<RGBAu8>(pCmdList, &m_target[GBufferTarget_RMA], size);
    graphics::loadTexture2D(pCmdList, &m_depthTarget, size, DepthStencilFormat_D24S8);
    m_resolution = size;

    // Setup the render target
    m_renderTarget = pCmdList->createRenderTarget(RenderTargetType_Texture);
    for (int64_t slot = 0; slot < GBufferTarget_Count; ++slot) {
      m_renderTarget->attachColour(m_target[slot], slot);
    }
    m_renderTarget->attachDepth(m_depthTarget);
    return true;
  }
}
