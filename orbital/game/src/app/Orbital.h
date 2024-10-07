#pragma once

#include "engine/Application.h"

namespace engine {
  class VirtualFileSystem;
  // class Windowing;
  class Rendering;
  class AssetManager;
  class LevelManager;
}

class Orbital : public engine::Application
{
public:
  Orbital();

  void function();

private:
  bfc::Ref<engine::VirtualFileSystem> m_pFileSystem;
  // bfc::Ref<engine::Windowing> m_pWindowing;
  bfc::Ref<engine::Rendering> m_pRendering;
  bfc::Ref<engine::AssetManager> m_pAssets;
  bfc::Ref<engine::LevelManager> m_pLevels;
};
