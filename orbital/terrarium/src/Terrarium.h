#pragma once

#include "Application.h"
#include "TerrariumGameSystems.h"

namespace engine {
  class VirtualFileSystem;
  // class Windowing;
  class Rendering;
  class AssetManager;
  class LevelManager;
  class LevelPlayer;
  class LevelEditor;
  class Input;

}

class TerrariumEditor;
class TerrariumGameSystems;

class Terrarium : public engine::Application
{
public:
  Terrarium();

private:
  // Core engine systems
  bfc::Ref<engine::Input>       m_pInput;
  bfc::Ref<engine::VirtualFileSystem> m_pFileSystem;
  // bfc::Ref<engine::Windowing> m_pWindowing;
  bfc::Ref<engine::Rendering> m_pRendering;
  bfc::Ref<engine::AssetManager> m_pAssets;
  bfc::Ref<engine::LevelManager> m_pLevels;
  // bfc::Ref<engine::LevelPlayer> m_pLevelPlayer;
  bfc::Ref<engine::LevelEditor> m_pLevelEditor;

  // Game systems
  bfc::Ref<TerrariumGameSystems> m_pGameSystems;
  bfc::Ref<TerrariumEditor> m_pTerrariumEditor;
};
