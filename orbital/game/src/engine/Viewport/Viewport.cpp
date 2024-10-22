#include "Viewport.h"
#include "Rendering/DeferredRenderer.h"
#include "Rendering/RenderScene.h"

using namespace bfc;

namespace engine {
  Viewport::Viewport(GraphicsDevice * pGraphics, AssetManager * pAssets, StringView const & viewportName)
    : m_events(String::format("Viewport.%.*s", viewportName.length(), viewportName.begin()))
    , m_renderScene(nullptr) {
    m_pGraphics = pGraphics;
    m_pRenderer = NewRef<DeferredRenderer>(pGraphics, pAssets);
  }

  void Viewport::render(bfc::GraphicsResource renderTarget) {
    if (m_renderScene.getLevel() == nullptr || getSize().x == 0 || getSize().y == 0)
      return;

    m_renderScene.setViews(collectViews(renderTarget));
    m_renderScene.collect();

    getGraphics()->bindRenderTarget(renderTarget);
    getGraphics()->clear({0, 0, 0, 255});
    getRenderer()->render(m_renderScene.views());
  }

  void Viewport::setSize(bfc::Vec2i const & size) {
    if (m_size == size) {
      return;
    }

    m_pRenderer->onResize(size);
    m_size = size;
  }

  void Viewport::setLevel(bfc::Ref<Level> const & pLevel) {
    m_renderScene = RenderScene(pLevel);
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
