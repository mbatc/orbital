#pragma once

#include "engine/Application.h"
#include "engine/Game.h"

namespace engine {
  class VirtualFileSystem;
  // class Windowing;
  class Rendering;
  class AssetManager;
  class LevelManager;
  class LevelPlayer;
  class LevelEditor;
}

class Orbital : public engine::Application
{
public:
  Orbital();

private:
  engine::Game m_game;

  bfc::Ref<engine::VirtualFileSystem> m_pFileSystem;
  // bfc::Ref<engine::Windowing> m_pWindowing;
  bfc::Ref<engine::Rendering> m_pRendering;
  bfc::Ref<engine::AssetManager> m_pAssets;
  bfc::Ref<engine::LevelManager> m_pLevels;
  bfc::Ref<engine::LevelPlayer> m_pLevelPlayer;
  bfc::Ref<engine::LevelEditor> m_pLevelEditor;
};
