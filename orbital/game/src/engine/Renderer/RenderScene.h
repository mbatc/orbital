#pragma once

#include "core/Span.h"
#include "core/Vector.h"

namespace bfc {
  class EventListener;
}

namespace engine {
  class Scene;
  class RenderData;
  class RenderView;
  class RenderScene;

  class RenderDataCollector {
  public:
    virtual void getRenderData(RenderView * pReviewView, Scene * pScene) = 0; 
  };
  class RenderScene {
  public:
    RenderScene(bfc::Ref<Scene> pScene);

    Scene * getScene() const;

    void setViews(bfc::Span<RenderView> const & views);

    bfc::Span<RenderView const> views() const;

  private:
    bfc::Vector<RenderData>   m_renderData;
    bfc::Vector<RenderView>   m_views;
    bfc::Ref<Scene>      m_pScene;
    bfc::Ref<RenderData> m_pRenderData;
  };
} // namespace engine
