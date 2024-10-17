#pragma once

#include "util/Settings.h"
// #include "LevelImporter.h"
#include "../Subsystem.h"

namespace bfc {
  class Mesh;
  class Material;
} // namespace bfc

namespace engine {
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

    bfc::Ref<Level> getActiveLevel() const;
    void            setActiveLevel(bfc::Ref<Level> const & pLevel);

    bool Import(Level * pLevel, bfc::URI const & uri) const;

  private:
    // Register the game component types.
    void registerComponentTypes();

    bfc::Ref<AssetManager>       m_pAssets      = nullptr; 
    bfc::Ref<Level>              m_pActiveLevel = nullptr;
    bfc::Ref<Rendering>          m_pRendering   = nullptr;
    bfc::Ref<bfc::EventListener> m_pInputs      = nullptr;
  };
} // namespace engine
