#include "LevelManager.h"
#include "LevelSerializer.h"
#include "Level.h"

#include "../Application.h"
#include "../Renderer/RenderData.h"
#include "../Renderer/RenderScene.h"
#include "../Renderer/Renderables.h"
#include "../Rendering.h"
#include "../assets/AssetManager.h"

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
    registerComponentTypes();

    auto pAssets = pApp->findSubsystem<AssetManager>();

    m_pRendering   = pApp->findSubsystem<Rendering>().get();
    m_pActiveLevel = NewRef<Level>();
    m_pRenderer    = NewRef<DeferredRenderer>(m_pRendering->getDevice(), pAssets.get());

    m_pInputs      = pApp->addListener();
    m_pInputs->on<events::KeyDown>([=](events::KeyDown const & kd) {
      if (kd.code == KeyCode_L) {
        BFC_LOG_INFO("LevelManager", "Reloading level");

        *m_pActiveLevel = {};
        LevelSerializer(pAssets.get()).deserialize(URI::File("D:/test.level"), *m_pActiveLevel);
      }
      return true;
    });

    URI levelPath = URI::File("D:/test.level");

    if (bfc::uriExists(levelPath)) {
      LevelSerializer(pAssets.get()).deserialize(URI::File("D:/test.level"), *m_pActiveLevel);
    } else {
      Filename skyboxPath = "game:skybox/nebula.skybox";
      EntityID testEntity = m_pActiveLevel->create();

      m_pActiveLevel->add<components::Name>(testEntity).name       = "Environment";
      m_pActiveLevel->add<components::Skybox>(testEntity).pTexture = pAssets->load<bfc::Texture>(URI::File(skyboxPath));

      components::Transform & sunTransform = m_pActiveLevel->add<components::Transform>(testEntity);
      sunTransform.lookAt(bfc::Vec3d(1, 1, -1));

      components::Light & sun = m_pActiveLevel->add<components::Light>(testEntity);
      sun.ambient             = {0.7f, 0.7f, 0.7f};
      sun.colour              = {0.7f, 0.7f, 0.7f};
      sun.castShadows         = true;

      Filename path       = "game:models/donut/dohey.obj";
      EntityID meshEntity = m_pActiveLevel->create();

      m_pActiveLevel->add<components::Name>(meshEntity).name = "The Dohey";

      components::Transform &  transform = m_pActiveLevel->add<components::Transform>(meshEntity);
      components::StaticMesh & mesh      = m_pActiveLevel->add<components::StaticMesh>(meshEntity);

      mesh.pMesh = pAssets->load<Mesh>(URI::File(path));
      for (int64_t i = 0; i < mesh.pMesh->getSubmeshCount(); ++i) {
        URI matUri = URI::File(String::format("%s#material.%lld", path.c_str(), i));
        mesh.materials.pushBack(pAssets->load<Material>(matUri));
      }
      mesh.castShadows = true;

      LevelSerializer(pAssets.get()).serialize(levelPath, *m_pActiveLevel);
    }

    return true;
  }

  void LevelManager::shutdown() {
    m_pActiveLevel = nullptr;
    m_pRenderer    = nullptr;
  }

  void LevelManager::loop() {
    Vec2i windowSize = m_pRendering->getMainWindow()->getSize();
    m_pRenderer->onResize(windowSize);

    RenderView  view;
    RenderScene scene(m_pActiveLevel);
    view.viewMatrix       = glm::inverse(glm::translate<double>(bfc::Vec3d{0, 0.2, 0.5}));
    view.projectionMatrix = glm::perspective<double>(glm::radians(60.0f), (float)windowSize.x / windowSize.y, 0.01f, 1000.0f);
    view.viewport     = {0, 0, 1, 1};
    view.renderTarget = InvalidGraphicsResource;

    scene.setViews({&view, 1});
    scene.collect();

    m_pRenderer->render(scene.views());
  }

  void LevelManager::registerComponentTypes() {
    registerComponentType<components::Name>("name");
    registerComponentType<components::Transform>("transform");
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
