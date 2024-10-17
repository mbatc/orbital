#pragma once

#include "core/Span.h"
#include "core/Vector.h"

#include "RenderData.h"
#include "Renderer.h"

namespace bfc {
  class EventListener;
}

namespace engine {
  class Level;
  class RenderData;
  class RenderView;
  class RenderScene;

  class RenderDataCollector {
  public:
    virtual void getRenderData(RenderView * pReviewView, Level const * pLevel) = 0; 
  };

  class RenderScene {
  public:
    RenderScene(bfc::Ref<Level> const & pLevel);

    Level * getLevel() const;

    void setViews(bfc::Span<RenderView> const & views);

    void collect();

    bfc::Span<RenderView const> views() const;

  private:
    bfc::Ref<Level>         m_pLevel;
    bfc::Vector<RenderData> m_renderData;
    bfc::Vector<RenderView> m_views;
  };
} // namespace engine
