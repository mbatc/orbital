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

  class RenderScene {
  public:
    RenderScene(bfc::GraphicsDevice * pDevice, bfc::Ref<Level> const & pLevel);

    Level * getLevel() const;

    void setViews(bfc::Span<RenderView> const & views);

    void collect();

    bfc::Span<RenderView const> views() const;

  private:
    bfc::GraphicsDevice *   m_pDevice;
    bfc::Ref<Level>         m_pLevel;
    bfc::Vector<bfc::Ref<RenderData>> m_renderData;
    bfc::Vector<RenderView> m_views;
  };
} // namespace engine
