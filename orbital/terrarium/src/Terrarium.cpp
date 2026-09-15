#include "Terrarium.h"

#include "Input.h"
#include "Assets/AssetManager.h"
#include "Levels/LevelManager.h"
#include "Rendering/Rendering.h"
#include "Editor/LevelEditor.h"
#include "TerrariumGameSystems.h"
#include "TerrariumEditor.h"

Terrarium::Terrarium()
  : Application({"Archive", "Terrarium" }) {
  bfc::URI repoBasePath     = bfc::URI::File(getBinaryPath()).resolveRelativeReference("../../../../../../");
  bfc::URI gameAssetsPath   = repoBasePath.resolveRelativeReference("orbital/terrarium/assets");
  bfc::URI engineAssetsPath = repoBasePath.resolveRelativeReference("orbital/engine/assets");

  m_pInput      = addSubsystem<engine::Input>();
  m_pFileSystem = addSubsystem<engine::VirtualFileSystem>(gameAssetsPath, engineAssetsPath);

  // m_pWindowing = addSubsystem<engine::Windowing>();
  m_pRendering       = addSubsystem<engine::Rendering>();
  m_pAssets          = addSubsystem<engine::AssetManager>();
  m_pLevels          = addSubsystem<engine::LevelManager>();
  m_pGameSystems     = addSubsystem<TerrariumGameSystems>();
  m_pLevelEditor     = addSubsystem<engine::LevelEditor>();
  m_pTerrariumEditor = addSubsystem<TerrariumEditor>();
  // m_pLevelPlayer = addSubsystem<engine::LevelPlayer>(&m_game);
}
