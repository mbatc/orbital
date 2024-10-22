#pragma once

#include "util/Settings.h"
#include "../Subsystem.h"

namespace bfc {
  class Mesh;
  class Material;
} // namespace bfc

namespace engine {
  class Level;
  enum SimulateState {
    SimulateState_Paused,
    SimulateState_Playing,
    SimulateState_Stopped,
    SimulateState_Count,
  };

  namespace events {
    struct OnLevelActivated {
      SimulateState mode;
      bfc::Ref<Level> pLevel = nullptr;
    };

    struct OnLevelPlay {
      bfc::Ref<Level> pLevel = nullptr;
    };

    struct OnLevelPause {
      bfc::Ref<Level> pLevel = nullptr;
    };

    struct OnLevelStop {
      bfc::Ref<Level> pLevel = nullptr;
    };

    struct OnLevelDeactivated {
      SimulateState mode;
      bfc::Ref<Level> pLevel = nullptr;
    };

    struct OnLevelUpdate {
      SimulateState  mode;
      Level *        pLevel    = nullptr;
      bfc::Timestamp deltaTime = 0;
    };
  } // namespace events

  class AssetManager;
  class Rendering;
  class Level;

  class LevelManager : public Subsystem {
  public:
    LevelManager();

    struct {
      bfc::Setting<bfc::URI> startupLevel;
    } settings;

    virtual bool init(Application * pApp) override;
    virtual void shutdown() override;
    virtual void loop(Application * pApp) override;

    void setSimulateState(SimulateState const & state);
    SimulateState getSimulateState() const;

    bool            load(bfc::URI const & uri, Level * pDst, bool merge = false) const;
    bfc::Ref<Level> load(bfc::URI const & uri) const;
    bool            save(bfc::URI const & uri, Level const &level) const;

    bfc::Ref<Level> getActiveLevel() const;
    void            setActiveLevel(bfc::Ref<Level> const & pLevel);

    bool Import(Level * pLevel, bfc::URI const & uri) const;

  private:
    // Register the game component types.
    void registerCoreComponentTypes();
    void registerCoreSystems();

    bfc::Ref<AssetManager>       m_pAssets      = nullptr;
    bfc::Ref<Rendering>          m_pRendering   = nullptr;
    bfc::Ref<bfc::EventListener> m_pInputs      = nullptr;

    bfc::Ref<Level> m_pActiveLevel = nullptr;
    bfc::Ref<Level> m_pBackupLevel = nullptr;

    SimulateState m_state = SimulateState_Stopped;
  };
} // namespace engine
