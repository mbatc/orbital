#include "Orbital.h"

#include "Input.h"
#include "Assets/AssetManager.h"
#include "Levels/LevelManager.h"
#include "Rendering/Rendering.h"
#include "Scripting/Scripting.h"
#include "Editor/LevelEditor.h"
#include "OrbitalLevelSystems.h"
#include "OrbitalEditor.h"

Orbital::Orbital()
  : Application({"Archive", "Orbital"}) {
  bfc::URI repoBasePath     = bfc::URI::File(getBinaryPath()).resolveRelativeReference("../../../../../../");
  bfc::URI gameAssetsPath   = repoBasePath.resolveRelativeReference("assets/game");
  bfc::URI engineAssetsPath = repoBasePath.resolveRelativeReference("assets/engine");

  m_pInput      = addSubsystem<engine::Input>();
  m_pFileSystem = addSubsystem<engine::VirtualFileSystem>(gameAssetsPath, engineAssetsPath);

  // m_pWindowing = addSubsystem<engine::Windowing>();
  m_pRendering     = addSubsystem<engine::Rendering>();
  m_pAssets        = addSubsystem<engine::AssetManager>();
  m_pLevels        = addSubsystem<engine::LevelManager>();
  m_pOrbitalLevels = addSubsystem<OrbitalGameSystems>();
  m_pScripting     = addSubsystem<engine::Scripting>();
  m_pLevelEditor   = addSubsystem<engine::LevelEditor>();
  m_pOrbitalEditor = addSubsystem<OrbitalEditor>();
  // m_pLevelPlayer = addSubsystem<engine::LevelPlayer>(&m_game);
}
