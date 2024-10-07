#include "render/PostProcessingStack.h"

namespace bfc {
  void PostProcessParams::bindInputs(GraphicsDevice * pDevice) const {
    pDevice->bindTexture(sceneColour, PostProcessInputBindPoint_SceneColour);
    pDevice->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_SceneColour);
    pDevice->bindTexture(pInput->sceneDepth, PostProcessInputBindPoint_SceneDepth);
    pDevice->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_SceneDepth);
    pDevice->bindTexture(pInput->baseColour, PostProcessInputBindPoint_BaseColour);
    pDevice->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_BaseColour);
    pDevice->bindTexture(pInput->ambientColour, PostProcessInputBindPoint_AmbientColour);
    pDevice->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_AmbientColour);
    pDevice->bindTexture(pInput->normal, PostProcessInputBindPoint_Normal);
    pDevice->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_Normal);
    pDevice->bindTexture(pInput->position, PostProcessInputBindPoint_Position);
    pDevice->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_Position);
    pDevice->bindTexture(pInput->rma, PostProcessInputBindPoint_RMA);
    pDevice->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_RMA);
  }

  void PostProcessParams::bindTarget(GraphicsDevice * pDevice) const {
    pDevice->bindRenderTarget(target);
    graphics::RenderTargetManager * pRT = pDevice->getRenderTargetManager();
    graphics::StateManager * pState = pDevice->getStateManager();
    pState->setViewport({0, 0}, pRT->getSize(target));
  }

  PostProcessingStack::PostProcessingStack(PostProcessingStack&& o) {
    std::swap(o.m_passes, m_passes);
    std::swap(o.m_intermediateSize, m_intermediateSize);
    std::swap(o.m_currentTarget, m_currentTarget);
    std::swap(o.m_intermediateColour[0], m_intermediateColour[0]);
    std::swap(o.m_intermediateColour[1], m_intermediateColour[1]);
    std::swap(o.m_intermediateTarget[1], m_intermediateTarget[1]);
    std::swap(o.m_intermediateTarget[1], m_intermediateTarget[1]);
  }

  void PostProcessingStack::addPass(std::function<void(GraphicsDevice *, PostProcessParams const &)> callback) {
    Pass newPass;
    newPass.callback = callback;
    m_passes.pushBack(newPass);
  }

  void PostProcessingStack::execute(GraphicsDevice * pDevice, Vec2i size) {
    // Create/Resize intermediate targets
    graphics::RenderTargetManager * pRT = pDevice->getRenderTargetManager();
    if (m_intermediateSize != size) {
      m_intermediateSize = size;
      for (int64_t i = 0; i < 2; ++i) {
        m_intermediateTarget[i] = {pDevice, pRT->createRenderTarget(RenderTargetType_Texture)};
        m_intermediateColour[i].load2D(pDevice, size, PixelFormat_RGBAf16);
        pRT->attachColour(m_intermediateTarget[i], m_intermediateColour[i]);
      }
    }

    // Setup passes.
    // Here we set the input/ouput of each pass to one of the intermediate textures in the ping-pong buffer.
    // The first pass input is set to the initial scene colour texture.
    // The last pass output is set to the post processing stack target.
    Texture src = sceneColour;
    for (Pass & pass : m_passes) {
      pass.params.pInput = &inputs;
      pass.params.sceneColour = src;
      pass.params.target = m_intermediateTarget[m_currentTarget];

      src = m_intermediateColour[m_currentTarget];
      m_currentTarget = (m_currentTarget + 1) % 2;
    }

    m_passes.back().params.target = target;

    graphics::StateManager * pState = pDevice->getStateManager();
    pState->setFeatureEnabled(GraphicsState_DepthTest, false);
    pState->setFeatureEnabled(GraphicsState_DepthWrite, false);
    pState->setFeatureEnabled(GraphicsState_StencilTest, false);
    pState->setFeatureEnabled(GraphicsState_Blend, false);

    // Run each pass
    for (Pass const & pass : m_passes) {
      pass.callback(pDevice, pass.params);
    }
  }

  void PostProcessingStack::reset() {
    m_passes.clear();
    m_currentTarget = 0;
  }
}
