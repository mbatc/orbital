#pragma once

#include "Subsystem.h"
#include "../Levels/LevelComponents.h"
#include "ui/Context.h"

namespace bfc {
  // class EventListener;
}

namespace engine {
  class Rendering;
  class Level;
  class LevelEditorViewport;
  class LevelManager;
  class LevelEditor : public Subsystem {
  public:
    LevelEditor();

    virtual bool init(Application * pApp) override;
    virtual void shutdown() override;
    virtual void loop(Application * pApp) override;

  private:
    void drawUI(bfc::Ref<LevelManager> const & pLevels);
    void drawTransformTree(bfc::Ref<Level> const & pLevel, EntityID entityID);
    void drawComponentProperties(bfc::Ref<Level> const & pLevel, EntityID entityID);
    void drawAddComponentMenu(bfc::Ref<Level> const & pLevel, EntityID targetEntityID);

    bfc::Ref<bfc::EventListener>  m_pViewportListener;
    bfc::Ref<bfc::EventListener>  m_pAppListener;

    bfc::Ref<LevelEditorViewport> m_pEditorViewport;
    bfc::Ref<LevelManager>        m_pLevels    = nullptr;
    bfc::Ref<AssetManager>        m_pAssets    = nullptr;
    bfc::Ref<Rendering>           m_pRendering = nullptr;

    EntityID m_selected = InvalidEntity;

    bfc::ui::Context m_uiContext;
    ImDrawData *     m_pDrawData = nullptr;
  };
}
