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

  void FeatureRenderer::onAdded(graphics::CommandList * pCmdList, Renderer * pRenderer) {
    BFC_UNUSED(pCmdList, pRenderer);
  }

  void FeatureRenderer::onResize(graphics::CommandList * pCmdList, Renderer * pRenderer, Vec2i const & size) {
    BFC_UNUSED(pCmdList, pRenderer, size);
  }

  void FeatureRenderer::beginFrame(graphics::CommandList * pCmdList, Renderer * pRenderer, Vector<RenderView> const & views) {
    BFC_UNUSED(pCmdList, pRenderer, views);
  }

  void FeatureRenderer::endFrame(graphics::CommandList * pCmdList, Renderer * pRenderer, Vector<RenderView> const & views) {
    BFC_UNUSED(pCmdList, pRenderer, views);
  }

  void FeatureRenderer::beginView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) {
    BFC_UNUSED(pCmdList, pRenderer, view);
  }

  void FeatureRenderer::renderView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) {
    BFC_UNUSED(pCmdList, pRenderer, view);
  }

  void FeatureRenderer::endView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) {
    BFC_UNUSED(pCmdList, pRenderer, view);
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

  void Renderer::render(bfc::graphics::CommandList * pCmdList, Vector<RenderView> const & views) {
    for (FeatureRenderer * pNewFeature : m_added) {
      pNewFeature->onAdded(pCmdList, this);
    }
    m_added.clear();

    for (FeatureRenderer * pFeature : m_features) {
      pFeature->beginFrame(pCmdList, this, views);
    }

    GraphicsDevice * pDevice = getGraphicsDevice();

    for (RenderView const & view : views) {
      /// Bind render target and set viewport
      pCmdList->bindRenderTarget(view.renderTarget);
      Vec2  renderTargetSize = view.renderTarget->getSize();
      Vec2i viewportPos      = Vec2i(renderTargetSize * Vec2{view.viewport.x, view.viewport.y});
      Vec2i viewportSize     = Vec2i(renderTargetSize * Vec2{view.viewport.z, view.viewport.w});

      pCmdList->pushState(graphics::State::Viewport{viewportPos, viewportSize});

      beginView(pCmdList, view);

      for (FeatureRenderer * pFeature : m_features) {
        pFeature->beginView(pCmdList, this, view);
      }

      for (FeatureRenderer * pFeature : m_features) {
        pFeature->renderView(pCmdList, this, view);
      }

      for (FeatureRenderer * pFeature : m_features) {
        pFeature->endView(pCmdList, this, view);
      }

      endView(pCmdList, view);

      pCmdList->popState();
    }

    for (FeatureRenderer * pFeature : m_features) {
      pFeature->endFrame(pCmdList, this, views);
    }
  }

  GraphicsDevice * Renderer::getGraphicsDevice() const {
    return m_pDevice;
  }

  void Renderer::beginView(bfc::graphics::CommandList * pCmdList, RenderView const & view) {
    BFC_UNUSED(pCmdList, view);
  }

  void Renderer::endView(bfc::graphics::CommandList * pCmdList, RenderView const & view) {
    BFC_UNUSED(pCmdList, view);
  }
} // namespace engine
