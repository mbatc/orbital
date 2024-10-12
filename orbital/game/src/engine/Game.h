#pragma once

#include "Levels/Level.h"
#include "Viewport.h"

namespace engine {
  enum SimulateState {
    SimulateState_Paused,
    SimulateState_Playing,
    SimulateState_Stopped,
    SimulateState_Count,
  };

  class GameViewport : public Viewport {
  public:
    GameViewport(bfc::GraphicsDevice * pGraphics, AssetManager * pAssets)
      : Viewport(pGraphics, pAssets, "Game") {}
  };

  class LevelEditorViewport : public Viewport {
  public:
    LevelEditorViewport(bfc::GraphicsDevice * pGraphics, AssetManager * pAssets)
      : Viewport(pGraphics, pAssets, "LevelEditor") {}
  };

  class Game {
  public:
    /// Set the simulate state for the game.
    bool setSimulate(SimulateState const & state);

    /// Get the current simulate state for the game.
    SimulateState getSimulate() const;

    /// Get the currently active level in the game.
    bfc::Ref<Level> getActiveLevel() const;

    /// Set the currently active level.
    void setActiveLevel(bfc::Ref<Level> const & newLevel);

    /// Get the editor level.
    bfc::Ref<Level> getEditorLevel() const;

    /// 
    void render();

    void update();

  private:
    SimulateState m_simulateState = SimulateState_Playing;

    // GameViewport        m_gameViewport;
    // LevelEditorViewport m_editorViewport;

    // Active level in the game. If playing, m_pEditorLevel is not m_pActiveLevel
    bfc::Ref<Level> m_pActiveLevel = nullptr;
    // Level being edited. Might separate this into a LevelEditor system
    bfc::Ref<Level> m_pEditorLevel = nullptr;

  };
}
