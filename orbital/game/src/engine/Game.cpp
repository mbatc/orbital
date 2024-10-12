#include "Game.h"

namespace engine {
  bool Game::setSimulate(SimulateState const & state) {
    if (state == m_simulateState) {
      return false;
    }

    if (m_simulateState == SimulateState_Stopped) {
      m_pActiveLevel = bfc::NewRef<Level>();
      // m_pEditorLevel->copyTo(m_pActiveLevel.get());
    }

    if (state == SimulateState_Stopped) {
      m_pActiveLevel = m_pEditorLevel;
    }

    // m_editorViewport.setLevel(m_pActiveLevel);
    // m_gameViewport.setLevel(m_pActiveLevel);

    m_simulateState = state;

    return true;
  }

  SimulateState Game::getSimulate() const {
    return m_simulateState;
  }

  bfc::Ref<Level> Game::getActiveLevel() const {
    return m_pActiveLevel;
  }

  void Game::setActiveLevel(bfc::Ref<Level> const & newLevel) {
    if (m_simulateState == SimulateState_Stopped) {
      m_pEditorLevel = newLevel;
      m_pActiveLevel = newLevel;
    } else {
      m_pActiveLevel = newLevel;
    }

    // m_editorViewport.setLevel(newLevel);
    // m_gameViewport.setLevel(newLevel);
  }

  bfc::Ref<Level> Game::getEditorLevel() const {
    return m_pEditorLevel;
  }
} // namespace engine
