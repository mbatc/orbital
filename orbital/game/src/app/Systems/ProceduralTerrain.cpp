#include "ProceduralTerrain.h"
#include "Levels/CoreComponents.h"
#include "Rendering/Rendering.h"
#include "Rendering/Renderer.h"
#include "Rendering/DeferredRenderer.h"
#include "Rendering/RenderData.h"
#include "Rendering/Renderables.h"

namespace {
  // class QuadTree {
  // public:
  //   void insert();
  //   void join();
  // 
  //   struct Node {
  //     int64_t  leaf = -1;
  //     uint64_t children[4];
  //   };
  // 
  //   struct Leaf {
  // 
  //   };
  // 
  //   bfc::Pool<Node> nodes;
  //   bfc::Pool<Leaf> leaves;
  // };
  //
  // struct StreamingContext {
  // 
  // };
  // 
  // bfc::Ref<StreamingContext> getStreamingContext() {
  //   return bfc::NewRef<StreamingContext>();
  // }

  struct ProceduralTerrainRenderable {
    bfc::Mat4d transform;
    uint32_t   seed      = 0;
    float      scale     = 1;
    float      minHeight = 0;
    float      maxHeight = 5;
    double     radius    = 0;
  };

  class ProceduralTerrainFeatureRenderer : public engine::FeatureRenderer {
  public:
    static constexpr int64_t TerrainBufferBindPoint = 4; // TODO: Register these with shader compiler. Shader compiler should provide the bind points.

    struct TerrainUBO {
      bfc::Vec2 sampleOffset = { 0, 0 };
      uint32_t  seed = 0;
      float     minHeight = 0;
      float     maxHeight = 5;
      float     scale = 1;
    };

    ProceduralTerrainFeatureRenderer(engine::AssetManager *pAssetManager)
      : m_terrainShader(pAssetManager, bfc::URI::File("engine:shaders/terrain/procedural-terrain.shader"))
      , m_quad(pAssetManager, bfc::URI::File("engine:models/primitives/plane.obj")) {}

    virtual void onResize(bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer, bfc::Vec2i const & size) {

    }

    virtual void beginFrame(bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer, bfc::Vector<engine::RenderView> const & views) {
      for (const auto & view : views) {
        view.getCameraFrustum();
      }
    }

    virtual void beginView(bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer, engine::RenderView const & view) override {

    }

    virtual void renderView(bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer, engine::RenderView const & view) override {
      auto pGBuffer = pRenderer->getResource<bfc::graphics::RenderTarget>(engine::DeferredRenderer::Resources::gbuffer);

      // TODO: Bind the gbuffer as the render target before rendering.
      //       How do I get the gbuffer target from the renderer?
      // pCmdList->bindRenderTarget(m_pGBuffer->getRenderTarget());

      bfc::graphics::StateManager * pState = pRenderer->getGraphicsDevice()->getStateManager();
      pCmdList->pushState(bfc::graphics::State::EnableDepthRead{true}, bfc::graphics::State::EnableDepthWrite{true}, bfc::graphics::State::EnableBlend{false});

      auto const & terrains = view.pRenderData->renderables<ProceduralTerrainRenderable>();
      pCmdList->bindRenderTarget(pGBuffer);
      pCmdList->bindProgram(m_terrainShader);
      pCmdList->bindVertexArray(m_quad->getVertexArray());
      pCmdList->bindUniformBuffer(m_terrainUBO, TerrainBufferBindPoint);
      pCmdList->bindUniformBuffer(m_modelUBO, bfc::renderer::BufferBinding_ModelBuffer);

      for (auto const& terrain : terrains) {
        int64_t tiles = (int64_t)ceil(terrain.radius);
        for (int64_t y = -tiles; y < tiles; ++y) {
          for (int64_t x = -tiles; x < tiles; ++x) {
            m_modelUBO.data.modelMatrix  = terrain.transform * glm::translate(bfc::Vec3d(x, 0, y));
            m_modelUBO.data.normalMatrix = bfc::renderer::calcNormalMatrix(terrain.transform);
            m_modelUBO.data.mvpMatrix    = bfc::renderer::calcMvpMatrix(terrain.transform, view.getViewProjectionMatrix());

            m_terrainUBO.data.seed         = terrain.seed;
            m_terrainUBO.data.scale        = terrain.scale;
            m_terrainUBO.data.maxHeight    = terrain.maxHeight;
            m_terrainUBO.data.minHeight    = terrain.minHeight;
            m_terrainUBO.data.sampleOffset = {x, y};

            m_modelUBO.upload(pCmdList);
            m_terrainUBO.upload(pCmdList);
            pCmdList->drawIndexed(std::numeric_limits<int64_t>::max(), 0, bfc::PrimitiveType_Patches);
          }
        }
      }

      pCmdList->bindRenderTarget(view.renderTarget);
      pCmdList->popState();
    }

    virtual void endView(bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer, engine::RenderView const & view) override {

    }

  private:
    engine::Asset<bfc::graphics::Program>       m_terrainShader;
    engine::Asset<bfc::Mesh>                    m_quad;
    bfc::graphics::StructuredBuffer<TerrainUBO> m_terrainUBO;
    bfc::graphics::StructuredBuffer<bfc::renderer::ModelBuffer> m_modelUBO;
  };

  class ProceduralTerrainRenderingExtension : public engine::IRenderingExtension {
  public:
    ProceduralTerrainRenderingExtension(bfc::Ref<engine::AssetManager> const & pAssets)
      : m_pAssets(pAssets) {}

    virtual void apply(engine::Renderer * pRenderer) override {
      pRenderer->addFeature<ProceduralTerrainFeatureRenderer>(engine::DeferredRenderer::Phase::Base::mesh, m_pAssets.get());
    }

    bfc::Ref<engine::AssetManager> m_pAssets;
  };
}

ProceduralTerrainSystem::ProceduralTerrainSystem(bfc::Ref<engine::Rendering> const & pRendering, bfc::Ref<engine::AssetManager> const & pAssets) {
  pRendering->registerExtension(bfc::NewRef<ProceduralTerrainRenderingExtension>(pAssets));
}

void ProceduralTerrainSystem::update(engine::Level * pLevel, bfc::Timestamp dt) {

}

void ProceduralTerrainSystem::play(engine::Level * pLevel) {

}

void ProceduralTerrainSystem::pause(engine::Level * pLevel) {

}

void ProceduralTerrainSystem::stop(engine::Level * pLevel) {

}

void ProceduralTerrainSystem::collectRenderData(engine::RenderView * pRenderView, engine::Level const * pLevel) {
  engine::RenderData & renderData = *pRenderView->pRenderData;
  auto &               terrains     = renderData.renderables<ProceduralTerrainRenderable>();

  for (auto const & [transform, planet] : pLevel->getView<components::Transform, components::ProceduralPlanet>()) {
    ProceduralTerrainRenderable renderable;
    renderable.transform = transform.globalTransform(pLevel);
    renderable.seed      = planet.seed;
    renderable.minHeight = planet.minHeight;
    renderable.maxHeight = planet.maxHeight;
    renderable.scale     = planet.scale;
    renderable.radius    = planet.radius;
    terrains.pushBack(renderable);
  }
}
