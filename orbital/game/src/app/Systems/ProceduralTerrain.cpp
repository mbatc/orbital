#include "ProceduralTerrain.h"
#include "Levels/CoreComponents.h"
#include "Rendering/Rendering.h"
#include "Rendering/Renderer.h"
#include "Rendering/DeferredRenderer.h"
#include "Rendering/RenderData.h"
#include "Rendering/Renderables.h"

namespace {
  class QuadTree {
  public:
    static uint64_t layerFromSize(double sz) {
      return uint64_t(std::log2(1.0 / sz));
    }

    bool insert(bfc::Vec3u64 const & coord) {
      return insert(coord, 0);
    }

    bool join(uint64_t node) {
      if (!nodes.isUsed(node)) {
        return false;
      }

      if (!nodes[node].split) {
        return false;
      }

      for (uint8_t i = 0; i < 4; ++i) {
        join(nodes[node].children[i]);

        nodes.erase(nodes[node].children[i]);
        nodes[node].children[i] = -1;
      }

      nodes[node].split = false;
      return true;
    }

    bool split(uint64_t node) {
      if (!nodes.isUsed(node)) {
        return false;
      }

      if (nodes[node].split) {
        return false;
      }

      uint64_t childNodes[] = {
        nodes.emplace(),
        nodes.emplace(),
        nodes.emplace(),
        nodes.emplace(),
      };
      std::memcpy(nodes[node].children, childNodes, sizeof(childNodes));
      auto & parent = nodes[node];
      parent.split = true;

      for (uint8_t i = 0; i < 4; ++i) {
        nodes[parent.children[i]].coord = parent.child(i);
      }
      return true;
    }

    struct Node {
      bfc::Vec3u64 coord;
      uint64_t     children[4];
      bool         split = false;

      int64_t layerDimensions() const {
        return 1ull << coord.z;
      }

      double size() const {
        return 1.0f / layerDimensions();
      }

      bfc::Vec3u64 parent() const {
        bfc::Vec3u64 parent = coord;
        parent.z -= 1;
        parent.x = parent.x / 2;
        parent.y = parent.y / 2;
        return parent;
      }

      bfc::Vec3u64 child(uint8_t index) const {
        bfc::Vec3u64 child = coord;
        child.z += 1;
        child.x = child.x * 2 + (index % 2);
        child.y = child.y * 2 + (index / 2);
        return child;
      }
    };

  private:
    bool isSplit(uint64_t node) {
      return nodes[node].split != -1;
    }

    bool insert(bfc::Vec3u64 const & coord, uint64_t root) {
      if (nodes[root].coord.z == coord.z)
        return nodes[root].coord == coord;


    }

    bfc::Pool<Node> nodes;
  };

  struct ProceduralTerrainRenderable {
    bfc::Mat4d transform;
    uint32_t   seed      = 0;
    float      scale     = 1;
    float      minHeight = 0;
    float      maxHeight = 5;
    double     radius    = 0;

    float      frequency = 1.0f;
    uint32_t   octaves   = 6;
    float      persistance = 0.5f;
    float      lacurnarity = 2.0f;
  };

  class ProceduralTerrainFeatureRenderer : public engine::FeatureRenderer {
  public:
    static constexpr int64_t TerrainBufferBindPoint = 4; // TODO: Register these with shader compiler. Shader compiler should provide the bind points.

    struct TerrainUBO {
      bfc::Vec2 sampleOffset = { 0, 0 };
      uint32_t  seed = 0;
      float     minHeight = 0;
      float     maxHeight = 5;

      // This stuff is probably going to be configured based on biome
      float     scale = 1;
      float     frequency;
      uint32_t  octaves;
      float     persistance;
      float     lacurnarity;
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
        int64_t nTiles = (int64_t)ceil(terrain.radius);

        auto terrainInverseTransform = glm::inverse(terrain.transform);
        auto cameraTerrainSpace      = bfc::Vec3d(terrainInverseTransform * bfc::Vec4d(view.getCameraPosition(), 1));
        auto terrainCenter           = glm::floor(cameraTerrainSpace);
        terrainCenter.y              = 0;

        for (int64_t y = -nTiles; y < nTiles; ++y) {
          for (int64_t x = -nTiles; x < nTiles; ++x) {
            const bfc::Vec3d tileCoord         = terrainCenter + bfc::Vec3d(x, 0, y);
            m_modelUBO.data.modelMatrix  = terrain.transform * glm::translate(tileCoord);
            m_modelUBO.data.normalMatrix = bfc::renderer::calcNormalMatrix(terrain.transform);
            m_modelUBO.data.mvpMatrix    = bfc::renderer::calcMvpMatrix(terrain.transform, view.getViewProjectionMatrix());

            m_terrainUBO.data.sampleOffset = {nTiles + tileCoord.x, nTiles + tileCoord.z};
            m_terrainUBO.data.seed         = terrain.seed;
            m_terrainUBO.data.scale        = terrain.scale;
            m_terrainUBO.data.maxHeight    = terrain.maxHeight;
            m_terrainUBO.data.minHeight    = terrain.minHeight;
            m_terrainUBO.data.frequency    = terrain.frequency;
            m_terrainUBO.data.octaves      = terrain.octaves;
            m_terrainUBO.data.persistance  = terrain.persistance;
            m_terrainUBO.data.lacurnarity  = terrain.lacurnarity;

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
    renderable.transform   = transform.globalTransform(pLevel);
    renderable.seed        = planet.seed;
    renderable.minHeight   = planet.minHeight;
    renderable.maxHeight   = planet.maxHeight;
    renderable.scale       = planet.scale;
    renderable.radius      = planet.radius;
    renderable.frequency   = planet.frequency;
    renderable.octaves     = planet.octaves;
    renderable.persistance = planet.persistance;
    renderable.lacurnarity = planet.lacurnarity;
    terrains.pushBack(renderable);
  }
}
