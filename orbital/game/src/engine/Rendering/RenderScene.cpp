#include "RenderScene.h"
#include "RenderData.h"
#include "Renderables.h"
#include "Renderer.h"
#include "platform/Events.h"
#include "../Levels/CoreComponents.h"
#include "../Levels/LevelSystem.h"

using namespace bfc;

namespace engine {
  class StaticMeshCollector : public ILevelRenderDataCollector {
  public:
    virtual void collectRenderData(RenderView * pReviewView, Level const * pLevel) override {
      RenderData *                                    pRenderData = pReviewView->pRenderData;
      RenderableStorage<MeshRenderable> &             meshes      = pRenderData->renderables<MeshRenderable>();
      RenderableStorage<MeshShadowCasterRenderable> & shadows     = pRenderData->renderables<MeshShadowCasterRenderable>();

      auto val = pLevel->getView<components::Transform, components::StaticMesh>();
      LevelView<components::Transform, components::StaticMesh> test(
        &pLevel->components<components::Transform>(),
        &pLevel->components<components::StaticMesh>()
      );

      for (auto & [transform, meshComponent] : pLevel->getView<components::Transform, components::StaticMesh>()) {
        if (meshComponent.pMesh == nullptr) {
          continue;
        }

        Mesh *     pMesh       = meshComponent.pMesh.get();
        Mat4d      modelMat    = transform.globalTransform(pLevel);
        Mat4d      normalMat   = glm::transpose(glm::inverse(modelMat));
        const bool castShadows = meshComponent.castShadows;

        for (int64_t i = 0; i < pMesh->getSubmeshCount(); ++i) {
          auto const & sm = pMesh->getSubMesh(i);

          geometry::Boxf bounds = sm.bounds;
          bounds.transform(modelMat);

          Material *     pMaterial = i < meshComponent.materials.size() ? meshComponent.materials[i].pMaterial.instance().get() : nullptr;

          MeshRenderable renderable;
          renderable.elementOffset = sm.elmOffset;
          renderable.elementCount  = sm.elmCount;
          renderable.modelMatrix   = modelMat;
          renderable.normalMatrix  = normalMat;
          renderable.vertexArray   = pMesh->getVertexArray();
          renderable.bounds        = bounds;
          renderable.shader        = i < meshComponent.materials.size() ? meshComponent.materials[i].pProgram.instance() : nullptr;
          renderable.primitiveType = meshComponent.useTesselation ? PrimitiveType_Patches : PrimitiveType_Triangle;

          if (pMaterial == nullptr) {
            renderable.materialBuffer = InvalidGraphicsResource;
            for (auto & texture : renderable.materialTextures) {
              texture = InvalidGraphicsResource;
            }
          } else {
            renderable.materialBuffer = *pMaterial;
            for (auto & [i, texture] : enumerate(pMaterial->textures)) {
              if (texture != nullptr) {
                renderable.materialTextures[i] = texture;
              }
            }
          }
          meshes.pushBack(renderable);

          if (castShadows) {
            MeshShadowCasterRenderable shadowCaster;
            shadowCaster.vertexArray   = pMesh->getVertexArray();
            shadowCaster.elementCount  = sm.elmCount;
            shadowCaster.elementOffset = sm.elmOffset;
            shadowCaster.bounds        = bounds;
            shadowCaster.modelMatrix   = modelMat;
            shadowCaster.normalMatrix  = normalMat;
            shadows.pushBack(shadowCaster);
          }
        }
      }
    }
  };
  
  class LightCollector : public ILevelRenderDataCollector {
  public:
    virtual void collectRenderData(RenderView * pReviewView, Level const * pLevel) override {
      RenderData * pRenderData = pReviewView->pRenderData;
  
      RenderableStorage<LightRenderable> & lights = pRenderData->renderables<LightRenderable>();
  
      for (auto & [transform, lightComponent] : pLevel->getView<components::Transform, components::Light>()) {
        Mat4d transformMat = transform.globalTransform(pLevel);
  
        LightRenderable renderable;
        renderable.type           = lightComponent.type;
        renderable.position       = transformMat[3];
        renderable.direction      = glm::normalize(Vec3d(glm::transpose(glm::inverse(transformMat)) * Vec4d(-math::up<double>, 0)));
        renderable.ambient        = lightComponent.ambient;
        renderable.colour         = lightComponent.colour;
        renderable.ambient        = lightComponent.ambient;
        renderable.attenuation    = lightComponent.attenuation;
        renderable.strength       = lightComponent.strength;
        renderable.innerConeAngle = lightComponent.innerConeAngle;
        renderable.outerConeAngle = lightComponent.outerConeAngle;
        renderable.castShadows    = lightComponent.castShadows;
        lights.pushBack(renderable);
      }
    }
  };
  
  class SkyboxCollector : public ILevelRenderDataCollector {
  public:
    virtual void collectRenderData(RenderView * pReviewView, Level const * pLevel) override {
      RenderData * pRenderData = pReviewView->pRenderData;
  
      RenderableStorage<CubeMapRenderable> & skyboxes = pRenderData->renderables<CubeMapRenderable>();
  
      for (auto & [skyboxComponent] : pLevel->getView<components::Skybox>()) {
        if (skyboxComponent.pTexture == nullptr) {
          continue;
        }
  
        CubeMapRenderable renderable;
        renderable.alpha   = 1.0f;
        renderable.texture = skyboxComponent.pTexture;
        skyboxes.pushBack(renderable);
      }
    }
  };
  
  class PostProcessCollector : public ILevelRenderDataCollector {
  public:
    virtual void collectRenderData(RenderView * pReviewView, Level const * pLevel) override {
      RenderData * pRenderData = pReviewView->pRenderData;

      auto & bloom    = pRenderData->renderables<PostProcessRenderable_Bloom>();
      auto & exposure = pRenderData->renderables<PostProcessRenderable_Exposure>();
      auto & ssao     = pRenderData->renderables<PostProcessRenderable_SSAO>();
      auto & ssr      = pRenderData->renderables<PostProcessRenderable_SSR>();
      
      for (auto & [volume] : pLevel->getView<components::PostProcessVolume>()) {
        if (!volume.infinite && !volume.extents.contains(pReviewView->getCameraPosition())) {
          continue;
        }
      
        // volume.target;
      
        EntityID id = pLevel->toEntity(&volume);
      
        if (auto * pBloom = pLevel->tryGet<components::PostProcess_Bloom>(id)) {
          PostProcessRenderable_Bloom renderable;
          renderable.filterRadius = pBloom->filterRadius;
          renderable.strength     = pBloom->strength;
          renderable.threshold    = pBloom->threshold;
          if (pBloom->dirt != nullptr) {
            renderable.dirtTex       = pBloom->dirt;
            renderable.dirtIntensity = pBloom->dirtIntensity;
          }
          bloom.pushBack(renderable);
        }
      
        if (auto * pTonemap = pLevel->tryGet<components::PostProcess_Tonemap>(id)) {
          PostProcessRenderable_Exposure renderable;
          renderable.exposure = pTonemap->exposure;
          exposure.pushBack(renderable);
        }
      
        if (auto * pSSAO = pLevel->tryGet<components::PostProcess_SSAO>(id)) {
          PostProcessRenderable_SSAO renderable;
          renderable.strength = pSSAO->strength;
          renderable.radius   = pSSAO->radius;
          renderable.bias     = pSSAO->bias;
          ssao.pushBack(renderable);
        }
      
        if (auto * pSSR = pLevel->tryGet<components::PostProcess_SSR>(id)) {
          PostProcessRenderable_SSR renderable;
          renderable.maxDistance = pSSR->maxDistance;
          renderable.resolution  = pSSR->resolution;
          renderable.steps       = pSSR->steps;
          renderable.thickness   = pSSR->thickness / pSSR->resolution;
          ssr.pushBack(renderable);
        }
      }
    }
  };

  RenderScene::RenderScene(bfc::GraphicsDevice * pDevice, bfc::Ref<Level> const & pLevel)
    : m_pLevel(pLevel)
    , m_pDevice(pDevice) {}

  Level * RenderScene::getLevel() const {
    return m_pLevel.get();
  }

  void RenderScene::setViews(Span<RenderView> const & views) {
    m_views = views;
    for (auto & [i, view] : enumerate(m_views)) {
      if (i >= m_renderData.size())
        m_renderData.pushBack(bfc::NewRef<RenderData>(m_pDevice));
      else
        m_renderData[i]->clear();
    }

    for (auto & [i, view] : enumerate(m_views)) {
      view.pRenderData = m_renderData[i].get();
      view.updateCachedProperties();
    }
  }

  void RenderScene::collect() {
    if (m_pLevel == nullptr) {
      return;
    }

    StaticMeshCollector  staticMeshes;
    LightCollector       lights;
    SkyboxCollector      skyboxes;
    PostProcessCollector pps;

    for (auto & view : m_views) {
      view.pRenderData->clear();

      staticMeshes.collectRenderData(&view, m_pLevel.get());
      lights.collectRenderData(&view, m_pLevel.get());
      skyboxes.collectRenderData(&view, m_pLevel.get());
      pps.collectRenderData(&view, m_pLevel.get());

      // Collect from extensions
      collectRenderData(&view, m_pLevel.get());
    }
  }

  Span<RenderView const> RenderScene::views() const {
    return m_views;
  }
} // namespace engine
