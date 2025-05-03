#include "Viewport.h"
#include "Rendering/DeferredRenderer.h"
#include "Rendering/RenderScene.h"

using namespace bfc;

namespace engine {
  Viewport::Viewport(graphics::CommandList * pCmdList, AssetManager * pAssets, StringView const & viewportName)
    : m_events(String::format("Viewport.%.*s", viewportName.length(), viewportName.begin()))
    , m_renderScene(pCmdList->getDevice(), nullptr) {
    m_pGraphics = pCmdList->getDevice();
    m_pRenderer = NewRef<DeferredRenderer>(pCmdList, pAssets);
  }

  void Viewport::render(bfc::graphics::CommandList *pCmdList, bfc::graphics::RenderTargetRef renderTarget) {
    if (m_renderScene.getLevel() == nullptr || getSize().x == 0 || getSize().y == 0)
      return;

    m_renderScene.setViews(collectViews(renderTarget));
    m_renderScene.collect();

    pCmdList->bindRenderTarget(renderTarget);
    pCmdList->clear({0, 0, 0, 255});
    getRenderer()->render(pCmdList, m_renderScene.views());
  }

  void Viewport::setSize(bfc::graphics::CommandList * pCmdList, bfc::Vec2i const & size) {
    if (m_size == size) {
      return;
    }

    m_pRenderer->onResize(pCmdList, size);
    m_size = size;
  }

  void Viewport::setLevel(bfc::Ref<Level> const & pLevel) {
    m_renderScene = RenderScene(m_pGraphics, pLevel);
  }

  Level * Viewport::getLevel() const {
    return m_renderScene.getLevel();
  }

  bfc::Vec2i Viewport::getSize() const {
    return m_size;
  }

  bfc::Events const * Viewport::getEvents() const {
    return &m_events;
  }

  bfc::Events * Viewport::getEvents() {
    return &m_events;
  }

  DeferredRenderer * Viewport::getRenderer() const {
    return m_pRenderer.get();
  }

  GraphicsDevice * Viewport::getGraphics() const {
    return m_pGraphics;
  }
} // namespace engine
