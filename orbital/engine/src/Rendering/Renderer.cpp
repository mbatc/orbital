#include "Renderer.h"
#include "RenderData.h"

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
    for (auto & [_, features] : m_phases) {
      for (FeatureRenderer *& pFeature : features) {
        delete pFeature;
      }
    }
    m_added.clear();
    m_phases.clear();
  }

  /// Get the number of phases in the renderer
  int64_t Renderer::numPhases() const {
    return m_phases.size();
  }

  bfc::StringView Renderer::getPhase(int64_t index) const {
    return m_phases.getKeys()[index];
  }

  int64_t Renderer::numFeatures(bfc::StringView const & phase) const {
    return m_phases[phase].size();
  }

  FeatureRenderer * Renderer::getFeature(bfc::StringView const & phase, int64_t index) const {
    return m_phases[phase][index];
  }

  bfc::Span<bfc::String> Renderer::getPhaseOrder() const {
    return m_phaseOrder.getView();
  }

  void Renderer::setPhaseOrder(bfc::Vector<bfc::String> const & phases) {
    m_phaseOrder = phases;
  }

  void Renderer::render(bfc::graphics::CommandList * pCmdList, Vector<RenderView> const & views) {
    for (RenderView const & view : views) {
      view.pRenderData->submitUploadList();
    }

    for (FeatureRenderer * pNewFeature : m_added) {
      pNewFeature->onAdded(pCmdList, this);
    }
    m_added.clear();

    for (auto & phase : m_phaseOrder) {
      for (FeatureRenderer * pFeature : m_phases[phase]) {
        pFeature->beginFrame(pCmdList, this, views);
      }
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

      for (auto & phase : m_phaseOrder) {
        for (FeatureRenderer * pFeature : m_phases[phase]) {
          pFeature->beginView(pCmdList, this, view);
        }
      }

      for (auto & phase : m_phaseOrder) {
        for (FeatureRenderer * pFeature : m_phases[phase]) {
          pFeature->renderView(pCmdList, this, view);
        }
      }

      for (auto & phase : m_phaseOrder) {
        for (FeatureRenderer * pFeature : m_phases[phase]) {
          pFeature->endView(pCmdList, this, view);
        }
      }

      endView(pCmdList, view);

      pCmdList->popState();
    }

    for (auto & phase : m_phaseOrder) {
      for (FeatureRenderer * pFeature : m_phases[phase]) {
        pFeature->endFrame(pCmdList, this, views);
      }
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
