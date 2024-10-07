#include "LevelManager.h"
#include "Application.h"
#include "DeferredRenderer/RenderData.h"
#include "DeferredRenderer/Renderables.h"
#include "Rendering.h"
#include "platform/Window.h"

#include "Level.h"
#include "assets/AssetManager.h"

#include "mesh/Mesh.h"

using namespace bfc;

namespace engine {
  LevelManager::LevelManager()
    : Subsystem(TypeID<LevelManager>(), "LevelManager") {}

  bool LevelManager::init(Application * pApp) {
    auto pAssets = pApp->findSubsystem<AssetManager>();
    m_pRendering = pApp->findSubsystem<Rendering>().get();

    Filename path = "game:models/donut/dohey.obj";
    // Filename path = "game:models/Sibenik Cathedral/sibenik.obj";

    m_pMesh = pAssets->load<Mesh>(URI::File(path));
    for (int64_t i = 0; i < m_pMesh->getSubmeshCount(); ++i) {
      URI matUri = URI::File(String::format("%s#material.%lld", path.c_str(), i));
      m_materials.pushBack(pAssets->load<Material>(matUri));
    }

    m_pActiveLevel = NewRef<Level>();
    m_pRenderer    = NewRef<DeferredRenderer>(m_pRendering->getDevice(), pAssets.get());

    return true;
  }

  void LevelManager::loop() {
    Vec2i windowSize = m_pRendering->getMainWindow()->getSize();
    m_pRenderer->onResize(windowSize);

    RenderData                                      data;
    RenderableStorage<MeshRenderable> &             meshes  = data.renderables<MeshRenderable>();
    RenderableStorage<MeshShadowCasterRenderable> & shadows = data.renderables<MeshShadowCasterRenderable>();
    RenderableStorage<LightRenderable> &            lights  = data.renderables<LightRenderable>();

    for (int64_t i = 0; i < m_pMesh->getSubmeshCount(); ++i) {
      auto const & sm = m_pMesh->getSubMesh(i);

      geometry::Boxf bounds    = sm.bounds;
      Material *     pMaterial = i < m_materials.size() ? m_materials[i].get() : nullptr;
      if (pMaterial == nullptr)
        continue;

      MeshRenderable renderable;
      renderable.elementOffset  = sm.elmOffset;
      renderable.elementCount   = sm.elmCount;
      renderable.modelMatrix    = glm::identity<Mat4d>();
      renderable.normalMatrix   = glm::identity<Mat4d>();
      renderable.materialBuffer = pMaterial == nullptr ? InvalidGraphicsResource : pMaterial->getResource();
      renderable.vertexArray    = m_pMesh->getVertexArray();
      renderable.bounds         = bounds;

      for (auto & [i, texture] : enumerate(pMaterial->textures)) {
        if (texture != nullptr) {
          renderable.materialTextures[i] = texture->getResource();
        }
      }
      meshes.pushBack(renderable);

      MeshShadowCasterRenderable caster;
      caster.bounds        = sm.bounds;
      caster.elementCount  = sm.elmCount;
      caster.elementOffset = sm.elmOffset;
      caster.modelMatrix   = glm::identity<Mat4d>();
      caster.normalMatrix  = glm::identity<Mat4d>();
      caster.vertexArray   = m_pMesh->getVertexArray();
      shadows.pushBack(caster);
    }

    LightRenderable l;
    l.type      = LightType_Sun;
    l.direction = glm::normalize(Vec3d(1, -1, -1));
    l.colour    = Vec3d(1, 1, 1);
    l.ambient   = Vec3d(0.3, 0.3, 0.3);
    l.strength  = 1;
    l.position  = {0, 0, 0};
    lights.pushBack(l);

    RenderView view;
    view.viewMatrix       = glm::inverse(glm::translate<double>(bfc::Vec3d{0, 0.2, 0.5}));
    view.projectionMatrix = glm::perspective<double>(glm::radians(60.0f), (float)windowSize.x / windowSize.y, 0.01f, 1000.0f);
    view.updateCachedProperties();
    view.viewport     = {0, 0, 1, 1};
    view.pRenderData  = &data;
    view.renderTarget = InvalidGraphicsResource;

    m_pRenderer->render({view});
  }
} // namespace engine
