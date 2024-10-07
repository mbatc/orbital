#include "RenderScene.h"
#include "RenderData.h"
#include "Renderables.h"
#include "Renderer.h"
#include "platform/Events.h"

using namespace bfc;

namespace engine {
    RenderScene::RenderScene(Ref<Scene> pScene)
      : m_pScene(pScene)
      , m_pRenderData(NewRef<RenderData>()) {
    }
    
    Scene * RenderScene::getScene() const {
      return m_pScene.get();
    }
    
    void RenderScene::setViews(Span<RenderView> const & views) {
      m_views = views;
      m_renderData.resize(m_views.size());
      for (auto & [i, view] : enumerate(m_views)) {
        view.pRenderData = m_renderData.begin() + i;
        view.updateCachedProperties();
      }
    }
    
    Span<RenderView const> RenderScene::views() const {
      return m_views;
    }

//  class MeshRenderSystem : public RenderDataCollector {
//  public:
//    virtual void getRenderData(RenderView * pReviewView, Scene * pScene) override {
//      RenderData *                                    pRenderData = pReviewView->pRenderData;
//      RenderableStorage<MeshRenderable> &             meshes      = pRenderData->renderables<MeshRenderable>();
//      RenderableStorage<MeshShadowCasterRenderable> & shadows     = pRenderData->renderables<MeshShadowCasterRenderable>();
//
//      for (auto & [transform, meshComponent] : pScene->getView<TransformComponent, MeshComponent>()) {
//        if (!meshComponent.mesh.load()) {
//          continue;
//        }
//
//        Mesh *     pMesh       = meshComponent.mesh.ptr();
//        Mat4d      modelMat    = transform.getGlobalTransform();
//        Mat4d      normalMat   = glm::transpose(glm::inverse(modelMat));
//        const bool castShadows = meshComponent.castShadows;
//        for (int64_t i = 0; i < pMesh->getSubmeshCount(); ++i) {
//          auto const & sm = pMesh->getSubMesh(i);
//          if (i >= meshComponent.materials.size() || !meshComponent.materials[i].load()) {
//            continue;
//          }
//
//          geometry::Boxf bounds = sm.bounds;
//          bounds.transform(modelMat);
//
//          Material * pMaterial = i < meshComponent.materials.size() ? meshComponent.materials[i].ptr() : nullptr;
//          if (pMaterial == nullptr)
//            continue;
//
//          MeshRenderable renderable;
//          renderable.elementOffset  = sm.elmOffset;
//          renderable.elementCount   = sm.elmCount;
//          renderable.modelMatrix    = modelMat;
//          renderable.normalMatrix   = normalMat;
//          renderable.materialBuffer = pMaterial->getResource();
//          renderable.vertexArray    = pMesh->getVertexArray();
//          renderable.bounds         = bounds;
//          for (auto & [i, texture] : enumerate(pMaterial->textures)) {
//            if (texture != nullptr) {
//              renderable.materialTextures[i] = texture->getResource();
//            }
//          }
//          meshes.pushBack(renderable);
//
//          if (castShadows) {
//            MeshShadowCasterRenderable shadowCaster;
//            shadowCaster.vertexArray   = pMesh->getVertexArray();
//            shadowCaster.elementCount  = sm.elmCount;
//            shadowCaster.elementOffset = sm.elmOffset;
//            shadowCaster.bounds        = bounds;
//            shadowCaster.modelMatrix   = modelMat;
//            shadowCaster.normalMatrix  = normalMat;
//            shadows.pushBack(shadowCaster);
//          }
//        }
//      }
//    }
//  };
//
//  class LightRenderSystem : public RenderDataCollector {
//  public:
//    virtual void getRenderData(RenderView * pReviewView, Scene * pScene) override {
//      RenderData * pRenderData = pReviewView->pRenderData;
//
//      RenderableStorage<LightRenderable> & lights = pRenderData->renderables<LightRenderable>();
//
//      for (auto & [transform, lightComponent] : pScene->getView<TransformComponent, LightComponent>()) {
//        Mat4d transformMat = transform.getGlobalTransform();
//
//        LightRenderable renderable;
//        renderable.type           = lightComponent.type;
//        renderable.position       = transformMat[3];
//        renderable.direction      = glm::normalize(Vec3d(glm::transpose(glm::inverse(transformMat)) * Vec4d(-math::up<double>, 0)));
//        renderable.ambient        = lightComponent.ambient;
//        renderable.colour         = lightComponent.colour;
//        renderable.ambient        = lightComponent.ambient;
//        renderable.attenuation    = lightComponent.attenuation;
//        renderable.strength       = lightComponent.strength;
//        renderable.innerConeAngle = lightComponent.innerConeAngle;
//        renderable.outerConeAngle = lightComponent.outerConeAngle;
//        renderable.castShadows    = lightComponent.castShadows;
//        lights.pushBack(renderable);
//      }
//    }
//  };
//
//  class SkyboxRenderSystem : public RenderDataCollector {
//  public:
//    virtual void getRenderData(RenderView * pReviewView, Scene * pScene) override {
//      RenderData * pRenderData = pReviewView->pRenderData;
//
//      RenderableStorage<SkyboxRenderable> & skyboxes = pRenderData->renderables<SkyboxRenderable>();
//
//      for (auto & [skyboxComponent] : pScene->getView<SkyboxComponent>()) {
//        if (!skyboxComponent.colour.load()) {
//          continue;
//        }
//
//        SkyboxRenderable renderable;
//        renderable.alpha   = 1.0f;
//        renderable.format  = skyboxComponent.type;
//        renderable.texture = *skyboxComponent.colour.ptr();
//        skyboxes.pushBack(renderable);
//      }
//    }
//  };
//
//  class PostProcessRenderSystem : public RenderDataCollector {
//  public:
//    virtual void getRenderData(RenderView * pReviewView, Scene * pScene) override {
//      RenderData * pRenderData = pReviewView->pRenderData;
//
//      auto & bloom    = pRenderData->renderables<PostProcessRenderable_Bloom>();
//      auto & exposure = pRenderData->renderables<PostProcessRenderable_Exposure>();
//      auto & ssao     = pRenderData->renderables<PostProcessRenderable_SSAO>();
//      auto & ssr      = pRenderData->renderables<PostProcessRenderable_SSR>();
//
//      for (auto & [volume] : pScene->getView<PostProcessVolumeComponent>()) {
//        if (!volume.infinite && !volume.extents.contains(pReviewView->getCameraPosition())) {
//          continue;
//        }
//
//        SceneEntity e = pScene->getEntity(volume);
//
//        if (auto * pBloom = e.tryGet<PostProcessComponent_Bloom>()) {
//          PostProcessRenderable_Bloom renderable;
//          renderable.filterRadius = pBloom->filterRadius;
//          renderable.strength     = pBloom->strength;
//          renderable.threshold    = pBloom->threshold;
//          if (pBloom->dirt.load()) {
//            renderable.dirtTex       = *pBloom->dirt.ptr();
//            renderable.dirtIntensity = pBloom->dirtIntensity;
//          }
//          bloom.pushBack(renderable);
//        }
//
//        if (auto * pTonemap = e.tryGet<PostProcessComponent_Tonemap>()) {
//          PostProcessRenderable_Exposure renderable;
//          renderable.exposure = pTonemap->exposure;
//          exposure.pushBack(renderable);
//        }
//
//        if (auto * pSSAO = e.tryGet<PostProcessComponent_SSAO>()) {
//          PostProcessRenderable_SSAO renderable;
//          renderable.strength = pSSAO->strength;
//          renderable.radius   = pSSAO->radius;
//          renderable.bias     = pSSAO->bias;
//          ssao.pushBack(renderable);
//        }
//
//        if (auto * pSSR = e.tryGet<PostProcessComponent_SSR>()) {
//          PostProcessRenderable_SSR renderable;
//          renderable.maxDistance = pSSR->maxDistance;
//          renderable.resolution  = pSSR->resolution;
//          renderable.steps       = pSSR->steps;
//          renderable.thickness   = pSSR->thickness / pSSR->resolution;
//          ssr.pushBack(renderable);
//        }
//      }
//    }
//  };

} // namespace engine
