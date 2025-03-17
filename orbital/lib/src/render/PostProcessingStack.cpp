#include "render/PostProcessingStack.h"

namespace bfc {
  void PostProcessParams::bindInputs(graphics::CommandList * pCmdList) const {
    pCmdList->bindTexture(sceneColour, PostProcessInputBindPoint_SceneColour);
    pCmdList->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_SceneColour);
    pCmdList->bindTexture(pInput->sceneDepth, PostProcessInputBindPoint_SceneDepth);
    pCmdList->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_SceneDepth);
    pCmdList->bindTexture(pInput->baseColour, PostProcessInputBindPoint_BaseColour);
    pCmdList->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_BaseColour);
    pCmdList->bindTexture(pInput->ambientColour, PostProcessInputBindPoint_AmbientColour);
    pCmdList->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_AmbientColour);
    pCmdList->bindTexture(pInput->normal, PostProcessInputBindPoint_Normal);
    pCmdList->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_Normal);
    pCmdList->bindTexture(pInput->position, PostProcessInputBindPoint_Position);
    pCmdList->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_Position);
    pCmdList->bindTexture(pInput->rma, PostProcessInputBindPoint_RMA);
    pCmdList->bindSampler(InvalidGraphicsResource, PostProcessInputBindPoint_RMA);
  }

  void PostProcessParams::bindTarget(graphics::CommandList * pCmdList) const {
    pCmdList->bindRenderTarget(target);
    pCmdList->setState(graphics::State::Viewport{{0, 0}, target->getSize()});
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

  void PostProcessingStack::addPass(std::function<void(graphics::CommandList *, PostProcessParams const &)> callback) {
    Pass newPass;
    newPass.callback = callback;
    m_passes.pushBack(newPass);
  }

  void PostProcessingStack::execute(graphics::CommandList * pCmdList, Vec2i size) {
    // Create/Resize intermediate targets
    if (m_intermediateSize != size) {
      m_intermediateSize = size;
      for (int64_t i = 0; i < 2; ++i) {
        m_intermediateTarget[i] = pCmdList->createRenderTarget(RenderTargetType_Texture);
        graphics::loadTexture2D(pCmdList, &m_intermediateColour[i], size, PixelFormat_RGBAf16);
        m_intermediateTarget[i]->attachColour(m_intermediateColour[i]);
      }
    }

    // Setup passes.
    // Here we set the input/ouput of each pass to one of the intermediate textures in the ping-pong buffer.
    // The first pass input is set to the initial scene colour texture.
    // The last pass output is set to the post processing stack target.
    graphics::TextureRef src = sceneColour;
    for (Pass & pass : m_passes) {
      pass.params.pInput = &inputs;
      pass.params.sceneColour = src;
      pass.params.target = m_intermediateTarget[m_currentTarget];

      src = m_intermediateColour[m_currentTarget];
      m_currentTarget = (m_currentTarget + 1) % 2;
    }

    m_passes.back().params.target = target;

    pCmdList->pushState(graphics::State::EnableDepthRead{false}, graphics::State::EnableDepthWrite{false}, graphics::State::EnableStencilTest{false},
                        graphics::State::EnableBlend{false});
    // Run each pass
    for (Pass const & pass : m_passes) {
      pass.callback(pCmdList, pass.params);
    }
    pCmdList->popState();
  }

  void PostProcessingStack::reset() {
    m_passes.clear();
    m_currentTarget = 0;
  }
}
