#include "LevelManager.h"
#include "LevelSerializer.h"
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
    settings.startupLevel = pApp->addSetting("level-manager/startup-level", URI::File("game:levels/main.level"));

    registerComponentTypes();

    m_pAssets = pApp->findSubsystem<AssetManager>();

    m_pRendering   = pApp->findSubsystem<Rendering>();
    m_pActiveLevel = NewRef<Level>();

    m_pInputs      = pApp->addListener();
    m_pInputs->on<events::KeyDown>([=](events::KeyDown const & kd) {
      if (kd.code == KeyCode_L) {
        BFC_LOG_INFO("LevelManager", "Reloading level");

        Level loaded;
        LevelSerializer(m_pAssets.get()).deserialize(settings.startupLevel.get(), loaded);
        m_pActiveLevel->clear();
        loaded.copyTo(m_pActiveLevel.get(), true);
      }
      return true;
    });

    // Load the startup level
    URI levelPath = settings.startupLevel.get();
    if (m_pAssets->getFileSystem()->exists(levelPath)) {
      LevelSerializer(m_pAssets.get()).deserialize(settings.startupLevel.get(), *m_pActiveLevel);
    } else {
      Filename skyboxPath = "game:skybox/nebula.skybox";
      EntityID testEntity = m_pActiveLevel->create();

      m_pActiveLevel->add<components::Name>(testEntity).name       = "Environment";
      m_pActiveLevel->add<components::Skybox>(testEntity).pTexture = m_pAssets->load<bfc::Texture>(URI::File(skyboxPath));

      components::Transform & sunTransform = m_pActiveLevel->add<components::Transform>(testEntity);
      sunTransform.lookAt(bfc::Vec3d(1, 1, -1));

      components::Light & sun = m_pActiveLevel->add<components::Light>(testEntity);
      sun.ambient             = {0.7f, 0.7f, 0.7f};
      sun.colour              = {0.7f, 0.7f, 0.7f};
      sun.castShadows         = true;

      LevelSerializer(m_pAssets.get()).serialize(levelPath, *m_pActiveLevel);
    }

    return true;
  }

  void LevelManager::shutdown() {
    m_pActiveLevel = nullptr;
  }

  bfc::Ref<Level> LevelManager::getActiveLevel() const {
    return m_pActiveLevel;
  }

  void LevelManager::setActiveLevel(bfc::Ref<Level> const & pLevel) {
    // TODO: Emit an event
    m_pActiveLevel = pLevel;
  }

  bool LevelManager::Import(Level * pLevel, bfc::URI const & uri) const {
    if (m_pAssets->canLoad<bfc::Mesh>(uri)) {
      Ref<Mesh> pMesh      = m_pAssets->load<Mesh>(uri);
      if (pMesh == nullptr) {
        return false;
      }

      EntityID meshEntity = m_pActiveLevel->create();

      m_pActiveLevel->add<components::Name>(meshEntity).name = Filename::name(uri.path(), false);

      components::Transform &  transform = m_pActiveLevel->add<components::Transform>(meshEntity);
      components::StaticMesh & mesh      = m_pActiveLevel->add<components::StaticMesh>(meshEntity);

      mesh.pMesh = pMesh;
      for (int64_t i = 0; i < mesh.pMesh->getSubmeshCount(); ++i) {
        URI matUri = uri.withFragment(String::format("material.%lld", i));
        mesh.materials.pushBack(m_pAssets->load<Material>(matUri));
      }
      return true;
    }

    return false;
  }

  void LevelManager::registerComponentTypes() {
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
} // namespace engine
