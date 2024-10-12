#include "Viewport.h"
#include "Renderer/DeferredRenderer.h"
#include "Renderer/RenderScene.h"

using namespace bfc;

namespace engine {
  Viewport::Viewport(GraphicsDevice * pGraphics, AssetManager * pAssets, StringView const & viewportName)
    : m_events(String::format("Viewport.%s", viewportName))
    , m_renderScene(nullptr) {
    m_mouse.attachTo(&m_events);
    m_keyboard.attachTo(&m_events);

    m_pGraphics    = pGraphics;
    m_pRenderer    = NewRef<DeferredRenderer>(pGraphics, pAssets);
    m_renderTarget = pGraphics->getRenderTargetManager()->createRenderTarget(RenderTargetType_Texture);
  }

  void Viewport::render() {
    if (m_renderScene.getLevel() == nullptr || getSize().x == 0 || getSize().y == 0)
      return;

    RenderView view;
    view.viewport         = {0, 0, 1, 1};
    view.viewMatrix       = m_viewMatrix;
    view.projectionMatrix = m_projectionMatrix;
    view.renderTarget     = getRenderTarget();

    // camera.get<CameraComponent>().calculateProjection({
    //   view.viewport.x * viewportSize.x,
    //   view.viewport.y * viewportSize.y,
    //   view.viewport.z * viewportSize.x,
    //   view.viewport.w * viewportSize.y
    // });

    m_renderScene.setViews({ &view, 1});
    m_renderScene.collect();

    getGraphics()->bindRenderTarget(getRenderTarget());
    getGraphics()->clear({0, 0, 0, 255});
    getRenderer()->render(m_renderScene.views());
  }

  void Viewport::setSize(bfc::Vec2i const & size) {
    if (m_size == size) {
      return;
    }
    m_colourTexture.load2D<RGBAu8>(m_pGraphics, size);
    m_depthTexture.load2D(m_pGraphics, size, DepthStencilFormat_D24S8);
    m_pGraphics->getRenderTargetManager()->attachColour(m_renderTarget, m_colourTexture);
    m_pGraphics->getRenderTargetManager()->attachDepth(m_renderTarget, m_depthTexture);
    m_pRenderer->onResize(size);
    m_size = size;
  }

  void Viewport::setLevel(bfc::Ref<Level> const & pLevel) {
    m_renderScene = RenderScene(pLevel);
  }

  bfc::Vec2i Viewport::getSize() const {
    return m_size;
  }

  const bfc::Mouse & Viewport::getMouse() const {
    return m_mouse;
  }

  const bfc::Keyboard & Viewport::getKeyboard() const {
    return m_keyboard;
  }

  bfc::Mouse & Viewport::getMouse() {
    return m_mouse;
  }

  bfc::Keyboard & Viewport::getKeyboard() {
    return m_keyboard;
  }

  bfc::Events const * Viewport::getEvents() const {
    return &m_events;
  }

  bfc::Events * Viewport::getEvents() {
    return &m_events;
  }

  void Viewport::setView(Mat4d const & viewMatrix) {
    m_viewMatrix = viewMatrix;
  }

  void Viewport::setProjection(Mat4d const & projectionMatrix) {
    m_projectionMatrix = projectionMatrix;
  }

  Mat4d Viewport::getView() const {
    return m_viewMatrix;
  }

  Mat4d Viewport::getProjection() const {
    return m_projectionMatrix;
  }

  DeferredRenderer * Viewport::getRenderer() const {
    return m_pRenderer.get();
  }

  GraphicsResource const & Viewport::getRenderTarget() const {
    return m_renderTarget;
  }

  Texture const & Viewport::getColourTexture() const {
    return m_colourTexture;
  }

  Texture const & Viewport::getDepthTexture() const {
    return m_depthTexture;
  }

  GraphicsDevice * Viewport::getGraphics() const {
    return m_pGraphics;
  }
} // namespace engine
