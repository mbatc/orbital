#include "render/GBuffer.h"

namespace bfc {
  GBuffer::GBuffer(GraphicsDevice * pDevice, Vec2i const & size)
    : m_pDevice(pDevice) {
    resize(size);
  }

  bool GBuffer::resize(Vec2i const & size) {
    if (m_resolution == size) {
      return false;
    }

    // Allocate textures
    m_target[GBufferTarget_BaseColour].load2D<RGBAu8>(m_pDevice, size);
    m_target[GBufferTarget_AmbientColour].load2D(m_pDevice, size, PixelFormat_RGBAf16);
    m_target[GBufferTarget_Position].load2D<RGBAf32>(m_pDevice, size);
    m_target[GBufferTarget_Normal].load2D<RGBAu8>(m_pDevice, size);
    m_target[GBufferTarget_RMA].load2D<RGBAu8>(m_pDevice, size);
    m_depthTarget.load2D(m_pDevice, size, DepthStencilFormat_D24S8);
    m_resolution = size;

    graphics::RenderTargetManager *pRTManager = m_pDevice->getRenderTargetManager();

    // Setup the render target
    m_renderTarget = { m_pDevice, pRTManager->createRenderTarget(RenderTargetType_Texture) };
    for (int64_t slot = 0; slot < GBufferTarget_Count; ++slot) {
      pRTManager->attachColour(m_renderTarget, m_target[slot], slot);
    }
    pRTManager->attachDepth(m_renderTarget, m_depthTarget);
    return true;
  }
}
