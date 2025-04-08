#include "LevelManager.h"
#include "LevelSerializer.h"
#include "LevelSystem.h"
#include "Level.h"

#include "Application.h"
#include "Rendering/RenderData.h"
#include "Rendering/RenderScene.h"
#include "Rendering/Renderables.h"
#include "Rendering/Rendering.h"
#include "Assets/AssetManager.h"

#include "mesh/Mesh.h"
#include "platform/Window.h"
#include "platform/OS.h"
#include "core/File.h"
#include "platform/Events.h"
#include "util/Log.h"

using namespace bfc;

namespace engine {
  LevelManager::LevelManager()
    : Subsystem(TypeID<LevelManager>(), "LevelManager") {}

  bool LevelManager::init(Application * pApp) {
    registerCoreComponentTypes();

    m_pAssets = pApp->findSubsystem<AssetManager>();

    m_pRendering   = pApp->findSubsystem<Rendering>();
    m_pActiveLevel = NewRef<Level>();

    return true;
  }

  void LevelManager::shutdown() {
    m_pActiveLevel = nullptr;
  }

  void LevelManager::loop(Application * pApp) {
    if (m_state == SimulateState_Playing) {
      updateLevel(getActiveLevel().get(), pApp->getDeltaTime());
    }
  }

  void LevelManager::setSimulateState(SimulateState const & newState) {
    if (newState == m_state && newState < SimulateState_Count)
      return;

    if (m_state == SimulateState_Stopped) {
      // Transition into a "playing" state
      BFC_LOG_INFO("LevelManager", "Start playing. Backing up level");
      Ref<Level> pPlayingLevel = NewRef<Level>();
      m_pActiveLevel->copyTo(pPlayingLevel.get(), true);
      m_pBackupLevel = m_pActiveLevel;
      setActiveLevel(pPlayingLevel);

      switch (newState) {
      case SimulateState_Playing: {
        playLevel(getActiveLevel().get());

        events::OnLevelPlay e;
        e.pLevel = getActiveLevel();
        getEvents()->broadcast(e);
      } break;
      }
    } else {
      switch (newState) {
      case SimulateState_Paused: {
        pauseLevel(getActiveLevel().get());

        events::OnLevelPause e;
        e.pLevel = getActiveLevel();
        getEvents()->broadcast(e);
      } break;
      case SimulateState_Playing: {
        playLevel(getActiveLevel().get());

        events::OnLevelPlay e;
        e.pLevel = getActiveLevel();
        getEvents()->broadcast(e);
      } break;
      case SimulateState_Stopped: {
        stopLevel(getActiveLevel().get());

        events::OnLevelStop e;
        e.pLevel = getActiveLevel();
        getEvents()->broadcast(e);
        setActiveLevel(m_pBackupLevel);
      } break;
      }
    }

    m_state = newState;
  }

  SimulateState LevelManager::getSimulateState() const {
    return m_state;
  }

  bool LevelManager::load(bfc::URI const & uri, Level * pDst, bool merge) const {
    return LevelSerializer(m_pAssets.get()).deserialize(uri, *pDst);
  }

  Ref<Level> LevelManager::load(bfc::URI const & uri) const {
    Ref<Level> pLevel = NewRef<Level>();

    return load(uri, pLevel.get()) ? pLevel : nullptr;
  }

  bool LevelManager::save(bfc::URI const & uri, Level const & level) const {
    return LevelSerializer(m_pAssets.get()).serialize(uri, level);
  }

  bfc::Ref<Level> LevelManager::getActiveLevel() const {
    return m_pActiveLevel;
  }

  void LevelManager::setActiveLevel(bfc::Ref<Level> const & pLevel) {
    if (m_pActiveLevel != nullptr) {
      engine::deactivateLevel(pLevel.get());

      events::OnLevelDeactivated e;
      e.mode = m_state;
      e.pLevel = m_pActiveLevel;
      getEvents()->broadcast(e);
    }

    m_pActiveLevel = pLevel;

    if (m_pActiveLevel != nullptr) {
      engine::activateLevel(pLevel.get());

      events::OnLevelActivated e;
      e.mode   = m_state;
      e.pLevel = m_pActiveLevel;
      getEvents()->broadcast(e);
    }
  }

  bool LevelManager::Import(Level * pLevel, bfc::URI const & uri) const {
    if (m_pAssets->canLoad<bfc::Mesh>(uri)) {
      Ref<Mesh> pMesh      = m_pAssets->load<Mesh>(uri);
      if (pMesh == nullptr) {
        return false;
      }

      EntityID meshEntity = m_pActiveLevel->create();

      m_pActiveLevel->add<components::Name>(meshEntity).name = Filename::name(uri.path(), false);

      components::Transform  & transform = m_pActiveLevel->add<components::Transform>(meshEntity);
      components::StaticMesh & mesh      = m_pActiveLevel->add<components::StaticMesh>(meshEntity);

      mesh.pMesh = pMesh;
      for (int64_t i = 0; i < mesh.pMesh->getSubmeshCount(); ++i) {
        URI matUri = uri.withFragment(String::format("material.%lld", i));
        mesh.materials.pushBack(components::ShadedMaterial{m_pAssets->load<Material>(matUri), nullptr});
      }
      return true;
    }

    return false;
  }

  void LevelManager::registerCoreComponentTypes() {
    registerComponentType<components::Name>("name");
    registerComponentType<components::Transform>("transform");
    registerComponentType<components::Camera>("camera");
    registerComponentType<components::Light>("light");
    registerComponentType<components::Skybox>("skybox");
    registerComponentType<components::StaticMesh>("static-mesh");
    registerComponentType<components::PostProcessVolume>("post-process-volume");
    registerComponentType<components::PostProcess_Tonemap>("post-process-tonemap");
    registerComponentType<components::PostProcess_Bloom>("post-process-bloom");
    registerComponentType<components::PostProcess_SSAO>("post-process-ssao");
    registerComponentType<components::PostProcess_SSR>("post-process-ssr");
  }

  void LevelManager::registerCoreSystems() {}
} // namespace engine
