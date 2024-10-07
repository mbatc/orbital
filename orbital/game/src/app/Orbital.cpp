#include "Orbital.h"

#include "engine/Assets/AssetManager.h"
#include "engine/LevelManager.h"
#include "engine/Rendering.h"

Orbital::Orbital()
  : Application({"Archive", "Orbital"}) {
  bfc::URI repoBasePath     = bfc::URI::File(getBinaryPath()).resolveRelativeReference("../../../../../../");
  bfc::URI gameAssetsPath   = repoBasePath.resolveRelativeReference("assets/game");
  bfc::URI engineAssetsPath = repoBasePath.resolveRelativeReference("assets/engine");

  m_pFileSystem = addSubsystem<engine::VirtualFileSystem>(gameAssetsPath, engineAssetsPath);

  // m_pWindowing = addSubsystem<engine::Windowing>();
  m_pRendering = addSubsystem<engine::Rendering>();
  m_pAssets    = addSubsystem<engine::AssetManager>();
  m_pLevels    = addSubsystem<engine::LevelManager>();
}
