#include "render/GBuffer.h"
#include "core/Array.h"
namespace bfc {
  namespace impl {
    static Array<std::function<void(graphics::CommandList * pCmdList, graphics::TextureRef *pTexture, Vec2i const &size)>, GBufferTarget_Count> loaders = {
      // GBufferTarget_BaseColour
      [](graphics::CommandList * pCmdList, graphics::TextureRef * pTexture, Vec2i const & size) {
        graphics::loadTexture2D<RGBAu8>(pCmdList, pTexture, size);
      },
      // GBufferTarget_AmbientColour
      [](graphics::CommandList * pCmdList, graphics::TextureRef * pTexture, Vec2i const & size) {
        graphics::loadTexture2D(pCmdList, pTexture, size, PixelFormat_RGBAf16);
      },
      // GBufferTarget_Position
      [](graphics::CommandList * pCmdList, graphics::TextureRef * pTexture, Vec2i const & size) {
        graphics::loadTexture2D<RGBAf32>(pCmdList, pTexture, size);
      },
      // GBufferTarget_Normal
      [](graphics::CommandList * pCmdList, graphics::TextureRef * pTexture, Vec2i const & size) {
        graphics::loadTexture2D<RGBAf32>(pCmdList, pTexture, size);
      },
      // GBufferTarget_RMA
      [](graphics::CommandList * pCmdList, graphics::TextureRef * pTexture, Vec2i const & size) {
        graphics::loadTexture2D<RGBAu8>(pCmdList, pTexture, size);
      },
    };
  }

  GBuffer::GBuffer(graphics::CommandList * pCmdList, Textures const & textures)
    : m_textures(textures) {
    resize(pCmdList, m_textures.resolution);
  }

  GBuffer::GBuffer(graphics::CommandList * pCmdList, Vec2i const & size)
    : GBuffer(pCmdList, Textures{size})
  {}

  bool GBuffer::resize(graphics::CommandList * pCmdList, Vec2i const & size) {
    // Allocate textures
    for (size_t i = 0; i < GBufferTarget_Count; ++i)
      if (m_textures.target[i] == nullptr || m_textures.target[i]->getSize() != Vec3i{size, 1})
        impl::loaders[i](pCmdList, &m_textures.target[i], size);
    if (m_textures.depthTarget == nullptr || m_textures.depthTarget->getSize() != Vec3i{size, 1})
      graphics::loadTexture2D(pCmdList, &m_textures.depthTarget, size, DepthStencilFormat_D24S8);
    m_textures.resolution = size;

    // Setup the render target
    m_renderTarget = pCmdList->createRenderTarget(RenderTargetType_Texture);
    for (int64_t slot = 0; slot < GBufferTarget_Count; ++slot) {
      m_renderTarget->attachColour(m_textures.target[slot], slot);
    }
    m_renderTarget->attachDepth(m_textures.depthTarget);
    return true;
  }
}
