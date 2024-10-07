#include "Renderer.h"

using namespace bfc;

namespace engine {
  void RenderView::updateCachedProperties() {
    m_camera.set(viewMatrix, projectionMatrix);
  }

  void RenderView::CameraCache::set(Mat4d view, Mat4d projection) {
    viewProjectionMatrix        = projection * view;
    inverseViewProjectionMatrix = glm::inverse(viewProjectionMatrix);
    inverseProjectionMatrix     = glm::inverse(projection);
    inverseViewMatrix           = glm::inverse(view);

    frustum = geometry::Frustum<double>(viewProjectionMatrix);

    right    = glm::normalize(Vec3d(inverseViewMatrix[0]));
    up       = glm::normalize(Vec3d(inverseViewMatrix[1]));
    fwd      = glm::normalize(Vec3d(inverseViewMatrix[2]));
    position = Vec3d(inverseViewMatrix[3]);
  }

  void FeatureRenderer::onAdded(Renderer * pRenderer) {
    BFC_UNUSED(pRenderer);
  }

  void FeatureRenderer::onResize(Renderer * pRenderer, Vec2i const & size) {
    BFC_UNUSED(pRenderer, size);
  }

  void FeatureRenderer::beginFrame(Renderer * pRenderer, Vector<RenderView> const & views) {
    BFC_UNUSED(pRenderer, views);
  }

  void FeatureRenderer::endFrame(Renderer * pRenderer, Vector<RenderView> const & views) {
    BFC_UNUSED(pRenderer, views);
  }

  void FeatureRenderer::beginView(Renderer * pRenderer, RenderView const & view) {
    BFC_UNUSED(pRenderer, view);
  }

  void FeatureRenderer::renderView(Renderer * pRenderer, RenderView const & view) {
    BFC_UNUSED(pRenderer, view);
  }

  void FeatureRenderer::endView(Renderer * pRenderer, RenderView const & view) {
    BFC_UNUSED(pRenderer, view);
  }

  Renderer::Renderer(GraphicsDevice * pDevice)
    : m_pDevice(pDevice) {}

  Renderer::~Renderer() {
    for (FeatureRenderer *& pFeature : m_features) {
      delete pFeature;
    }
    m_added.clear();
    m_features.clear();
  }

  int64_t Renderer::numFeatures() const {
    return m_features.size();
  }

  FeatureRenderer * Renderer::getFeature(int64_t index) const {
    return m_features[index];
  }

  void Renderer::render(Vector<RenderView> const & views) {
    for (FeatureRenderer * pNewFeature : m_added) {
      pNewFeature->onAdded(this);
    }
    m_added.clear();

    for (FeatureRenderer * pFeature : m_features) {
      pFeature->beginFrame(this, views);
    }

    GraphicsDevice * pDevice = getGraphicsDevice();
    for (RenderView const & view : views) {
      /// Bind render target and set viewport
      graphics::RenderTargetManager * pRenderTargets = pDevice->getRenderTargetManager();
      graphics::StateManager *        pState         = pDevice->getStateManager();

      pDevice->bindRenderTarget(view.renderTarget);
      Vec2  renderTargetSize = pRenderTargets->getSize(view.renderTarget);
      Vec2i viewportPos      = Vec2i(renderTargetSize * Vec2{view.viewport.x, view.viewport.y});
      Vec2i viewportSize     = Vec2i(renderTargetSize * Vec2{view.viewport.z, view.viewport.w});

      pState->setViewport(viewportPos, viewportSize);

      beginView(view);

      for (FeatureRenderer * pFeature : m_features) {
        pFeature->beginView(this, view);
      }

      for (FeatureRenderer * pFeature : m_features) {
        pFeature->renderView(this, view);
      }

      for (FeatureRenderer * pFeature : m_features) {
        pFeature->endView(this, view);
      }

      endView(view);
    }

    for (FeatureRenderer * pFeature : m_features) {
      pFeature->endFrame(this, views);
    }
  }

  GraphicsDevice * Renderer::getGraphicsDevice() const {
    return m_pDevice;
  }

  void Renderer::beginView(RenderView const & view) {
    BFC_UNUSED(view);
  }

  void Renderer::endView(RenderView const & view) {
    BFC_UNUSED(view);
  }
} // namespace engine
