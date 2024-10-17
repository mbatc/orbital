#pragma once

#include "Subsystem.h"
#include "ui/Context.h"

namespace bfc {
  // class EventListener;
}

namespace engine {
  enum SimulateState {
    SimulateState_Paused,
    SimulateState_Playing,
    SimulateState_Stopped,
    SimulateState_Count,
  };

  class Level;
  class LevelEditorViewport;
  class LevelEditor : public Subsystem {
  public:
    LevelEditor();

    virtual bool init(Application * pApp) override;
    virtual void shutdown() override;
    virtual void loop(Application * pApp) override;

  private:
    bfc::Ref<bfc::EventListener>  m_pViewportListener;
    bfc::Ref<LevelEditorViewport> m_pViewport;
    bfc::Ref<Level>               m_pEditorLevel = nullptr;
    bfc::ui::Context m_uiContext;
  };
}
