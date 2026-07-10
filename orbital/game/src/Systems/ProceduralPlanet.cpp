#include "ProceduralPlanet.h"
#include "Levels/CoreComponents.h"
#include "Rendering/DeferredRenderer.h"
#include "Rendering/RenderData.h"
#include "Rendering/Renderables.h"
#include "Rendering/Renderer.h"
#include "Rendering/Rendering.h"
#include "dsa/QuadTree.h"

namespace {
  static bfc::Mat4d faceTransforms[bfc::CubeMapFace_Count] = {
    bfc::math::translation(bfc::math::right<double> * 0.5) *
      glm::toMat4(glm::angleAxis(glm::half_pi<double>(), bfc::math::forward<double>)), // CubeMapFace_Right,

    bfc::math::translation(-bfc::math::right<double> * 0.5) *
      glm::toMat4(glm::angleAxis(-glm::half_pi<double>(), bfc::math::forward<double>)), // CubeMapFace_Left,

    bfc::math::translation(bfc::math::up<double> * 0.5), // CubeMapFace_Top,

    bfc::math::translation(-bfc::math::up<double> * 0.5) * glm::toMat4(glm::angleAxis(glm::pi<double>(), bfc::math::right<double>)), // CubeMapFace_Bottom,

    bfc::math::translation(bfc::math::forward<double> * 0.5) *
      glm::toMat4(glm::angleAxis(-glm::half_pi<double>(), bfc::math::right<double>)), // CubeMapFace_Front

    bfc::math::translation(-bfc::math::forward<double> * 0.5) *
      glm::toMat4(glm::angleAxis(glm::half_pi<double>(), bfc::math::right<double>)), // CubeMapFace_Back,
  };

  struct ProceduralPlanetRenderable {
    engine::EntityID planetID = 0;

    bfc::Mat4d transform;
    uint32_t   seed      = 0;
    float      scale     = 1;
    float      minHeight = 0;
    float      maxHeight = 5;
    double     radius    = 0;

    uint32_t octaves     = 6;
    float    persistance = 0.5f;
    float    lacurnarity = 2.0f;
  };

  struct PlanetAtmosphereRenderable {
    engine::EntityID planetID;
  };

  // Per tile data
  struct PlanetTerrainTileUBO {
    bfc::Mat4 sampleTransform = glm::identity<bfc::Mat4d>();
    float     tileSize        = 1;
    float     padding0[3];
  };

  // Per planet data
  struct PlanetTerrainUBO {
    uint32_t seed      = 0;
    float    minHeight = 0;
    float    maxHeight = 5;

    // This stuff is probably going to be configured based on biome
    float    scale = 1;
    float    frequency;
    uint32_t octaves;
    float    persistance;
    float    lacurnarity;
  };

  static constexpr int64_t TerrainBufferBindPoint     = 4; // TODO: Register these with shader compiler. Shader compiler should provide the bind points.
  static constexpr int64_t TerrainTileBufferBindPoint = 5; // TODO: Register these with shader compiler. Shader compiler should provide the bind points.

  class ProceduralPlanetTerrainFeatureRenderer : public engine::FeatureRenderer {
  public:
    ProceduralPlanetTerrainFeatureRenderer(engine::AssetManager * pAssetManager)
      : m_terrainShader(pAssetManager, bfc::URI::File("engine:shaders/terrain/procedural-planet-terrain.shader"))
      , m_waterTransmittanceShader(pAssetManager, bfc::URI::File("engine:shaders/terrain/planet-water-transmittance.shader"))
      , m_quad(pAssetManager, bfc::URI::File("engine:models/primitives/plane.obj")) {}

    virtual void onResize(bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer, bfc::Vec2i const & size) {}

    virtual void beginFrame(bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer, bfc::Vector<engine::RenderView> const & views) {
      m_viewData.clear();
    }

    virtual void beginView(bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer, engine::RenderView const & view) override {
      auto const & terrains = view.pRenderData->renderables<ProceduralPlanetRenderable>();

      const uint64_t viewID = view.getViewID();
      if (m_viewData.contains(viewID)) {
        return;
      }

      ViewData viewData;
      for (auto const & terrain : terrains) {
        const auto   terrainInverseTransform = glm::inverse(terrain.transform);
        const auto   camPosTerrainSpace      = bfc::Vec3d(terrainInverseTransform * bfc::Vec4d(view.getCameraPosition(), 1));
        const double terrainHeight           = glm::length(camPosTerrainSpace) - 1;

        // Project camera position to the unit cube
        const auto maxComponent   = bfc::math::maxComponent(bfc::math::abs(camPosTerrainSpace));
        const auto camPosUnitCube = maxComponent < std::numeric_limits<double>::epsilon() ? bfc::Vec3d(0) : (camPosTerrainSpace / maxComponent);

        for (bfc::Mat4d const & side : faceTransforms) {
          Instance instance;
          instance.side           = side;
          instance.modelTransform = terrain.transform;
          instance.terrain        = terrain;

          auto terrainSideInverseTransform = glm::inverse(side * glm::translate(-bfc::Vec3d(0.5, 0, 0.5)));
          auto cameraTerrainSpace          = bfc::Vec3d(terrainSideInverseTransform * bfc::Vec4d(camPosUnitCube, 1));
          std::swap(cameraTerrainSpace.y, cameraTerrainSpace.z);

          if (cameraTerrainSpace.z > 0)
            cameraTerrainSpace.z = terrainHeight;

          instance.tree.trySplit(
            [&](QuadTree::Node const & node) {
              if (node.coord.z >= 20)
                return false;
              const double sz     = node.size();
              const double halfSz = sz / 2;
              const auto   center = bfc::Vec3d(node.coord.x, node.coord.y, 0) * sz + bfc::Vec3d(halfSz, halfSz, 0);
              const double dist2  = glm::length2(center - cameraTerrainSpace);
              return dist2 < sz * sz * 1.5 * 1.5;
            },
            0, true);

          viewData.instances.pushBack(std::move(instance));
        }
      }

      m_viewData.add(viewID, std::move(viewData));
    }

    virtual void onRenderRequest(std::any const & request, bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer,
                                 engine::RenderView const & view) {
      if (auto * pBasePassRequest = std::any_cast<engine::DeferredRenderer::Stages::BasePassRequest>(&request))
        onBasePass(*pBasePassRequest, pCmdList, pRenderer, view);
      if (auto * pTransparencyDepth = std::any_cast<engine::DeferredRenderer::Stages::Transparency::Depth>(&request))
        onTransparencyDepth(*pTransparencyDepth, pCmdList, pRenderer, view);
      if (auto * pTransparencyTransmittance = std::any_cast<engine::DeferredRenderer::Stages::Transparency::Transmittance>(&request))
        onTransparencyTransmittance(*pTransparencyTransmittance, pCmdList, pRenderer, view);
    }

    void onBasePass(engine::DeferredRenderer::Stages::BasePassRequest const & request, bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer,
                    engine::RenderView const & view) {
      uint64_t viewID = view.getViewID();
      if (!m_viewData.contains(viewID)) {
        return;
      }

      bfc::graphics::StateManager * pState = pRenderer->getGraphicsDevice()->getStateManager();
      pCmdList->pushState(bfc::graphics::State::EnableDepthRead{true}, bfc::graphics::State::EnableDepthWrite{true}, bfc::graphics::State::EnableBlend{false});
      pCmdList->bindProgram(m_terrainShader);

      renderTerrainTiles(pCmdList, viewID, view.getViewProjectionMatrix());
    }

    void onTransparencyDepth(engine::DeferredRenderer::Stages::Transparency::Depth const & request, bfc::graphics::CommandList * pCmdList,
                             engine::Renderer * pRenderer, engine::RenderView const & view) {
      uint64_t viewID = view.getViewID();
      if (!m_viewData.contains(viewID)) {
        return;
      }

      pCmdList->bindProgram(m_waterTransmittanceShader);

      bfc::Vec3 sampleOffset = bfc::Vec3(std::sinf((float)bfc::Timestamp::now().secs() / 1000));
      pCmdList->setUniform("sampleOffset0", sampleOffset);

      renderTerrainTiles(pCmdList, viewID, view.getViewProjectionMatrix());
    }

    void onTransparencyTransmittance(engine::DeferredRenderer::Stages::Transparency::Transmittance const & request, bfc::graphics::CommandList * pCmdList,
                         engine::Renderer * pRenderer, engine::RenderView const & view) {
      uint64_t viewID = view.getViewID();
      if (!m_viewData.contains(viewID)) {
        return;
      }

      for (int i = 0; i < bfc::GBufferTarget_Count; ++i) {
        pCmdList->bindTexture((*request.pOpaqueGBuffer)[i], 8 + i);
      }

      pCmdList->bindProgram(m_waterTransmittanceShader);

      renderTerrainTiles(pCmdList, viewID, view.getViewProjectionMatrix());

      for (int i = 0; i < bfc::GBufferTarget_Count; ++i) {
        pCmdList->bindTexture(nullptr, 8 + i);
      }
    }

    virtual void endView(bfc::graphics::CommandList * pCmdList, engine::Renderer * pRenderer, engine::RenderView const & view) override {}

  private:
    void renderTerrainTiles(bfc::graphics::CommandList * pCmdList, uint64_t viewID, bfc::Mat4 viewProjectionMatrix)
    {
      pCmdList->bindVertexArray(m_quad->getVertexArray());
      pCmdList->bindUniformBuffer(m_terrainTileUBO, TerrainTileBufferBindPoint);
      pCmdList->bindUniformBuffer(m_terrainUBO, TerrainBufferBindPoint);
      pCmdList->bindUniformBuffer(m_modelUBO, bfc::renderer::BufferBinding_ModelBuffer);

      for (auto & instance : m_viewData[viewID].instances) {
        m_modelUBO.data.modelMatrix  = instance.terrain.transform;
        m_modelUBO.data.normalMatrix = bfc::renderer::calcNormalMatrix(instance.terrain.transform);
        m_modelUBO.data.mvpMatrix    = bfc::renderer::calcMvpMatrix(instance.terrain.transform, viewProjectionMatrix);
        m_modelUBO.upload(pCmdList);

        m_terrainUBO.data.seed        = instance.terrain.seed;
        m_terrainUBO.data.scale       = instance.terrain.scale;
        m_terrainUBO.data.maxHeight   = instance.terrain.maxHeight;
        m_terrainUBO.data.minHeight   = instance.terrain.minHeight;
        m_terrainUBO.data.frequency   = 1.0f;
        m_terrainUBO.data.octaves     = instance.terrain.octaves;
        m_terrainUBO.data.persistance = instance.terrain.persistance;
        m_terrainUBO.data.lacurnarity = instance.terrain.lacurnarity;
        m_terrainUBO.upload(pCmdList);

        instance.tree.forLeaves([=](QuadTree::Node const & node) {
          const double     size            = node.size();
          const bfc::Vec3d tileCoord       = bfc::Vec3d(node.coord.x, 0, node.coord.y) * size;
          const bfc::Mat4d sampleTransform = instance.side * glm::translate(-bfc::Vec3d(0.5, 0, 0.5)) *
                                             glm::translate(tileCoord) * glm::scale(bfc::Vec3d(size, 1, size)) *
                                             glm::translate(bfc::Vec3d(0.5, 0, 0.5));

          m_terrainTileUBO.data.tileSize        = (float)size;
          m_terrainTileUBO.data.sampleTransform = sampleTransform;
          m_terrainTileUBO.upload(pCmdList);

          pCmdList->drawIndexed(std::numeric_limits<int64_t>::max(), 0, bfc::PrimitiveType_Patches);
        });
      }
    }

    engine::Asset<bfc::graphics::Program> m_terrainShader;
    engine::Asset<bfc::graphics::Program> m_waterTransmittanceShader;

    struct Instance {
      ProceduralPlanetRenderable terrain;
      bfc::Mat4d                 modelTransform;
      bfc::Mat4d                 side;
      QuadTree                   tree;
    };

    struct ViewData {
      bfc::Vector<Instance> instances;
    };

    engine::Asset<bfc::Mesh>     m_quad;
    bfc::Map<uint64_t, ViewData> m_viewData;

    bfc::graphics::StructuredBuffer<PlanetTerrainTileUBO>       m_terrainTileUBO;
    bfc::graphics::StructuredBuffer<PlanetTerrainUBO>           m_terrainUBO;
    bfc::graphics::StructuredBuffer<bfc::renderer::ModelBuffer> m_modelUBO;
  };

  class PlanetAtmosphereFeatureRenderer : public engine::FeatureRenderer {
  public:
    PlanetAtmosphereFeatureRenderer(engine::AssetManager *) {}

    engine::Asset<bfc::graphics::Program> m_atmosphereShader;

    engine::Asset<bfc::Mesh>                                    m_quad;
    bfc::graphics::StructuredBuffer<PlanetTerrainUBO>           m_terrainUBO;
    bfc::graphics::StructuredBuffer<bfc::renderer::ModelBuffer> m_modelUBO;
  };

  class ProceduralTerrainRenderingExtension : public engine::IRenderingExtension {
  public:
    ProceduralTerrainRenderingExtension(bfc::Ref<engine::AssetManager> const & pAssets)
      : m_pAssets(pAssets) {}

    virtual void apply(engine::Renderer * pRenderer) override {
      pRenderer->addFeature<ProceduralPlanetTerrainFeatureRenderer>(engine::DeferredRenderer::Phase::Base::Mesh::opaque, m_pAssets.get());
    }

    bfc::Ref<engine::AssetManager> m_pAssets;
  };
} // namespace

ProceduralTerrainSystem::ProceduralTerrainSystem(bfc::Ref<engine::Rendering> const & pRendering, bfc::Ref<engine::AssetManager> const & pAssets) {
  pRendering->registerExtension(bfc::NewRef<ProceduralTerrainRenderingExtension>(pAssets));
}

void ProceduralTerrainSystem::update(engine::Level * pLevel, bfc::Timestamp dt) {}

void ProceduralTerrainSystem::play(engine::Level * pLevel) {}

void ProceduralTerrainSystem::pause(engine::Level * pLevel) {}

void ProceduralTerrainSystem::stop(engine::Level * pLevel) {}

void ProceduralTerrainSystem::collectRenderData(engine::RenderView * pRenderView, engine::Level const * pLevel) {
  engine::RenderData & renderData = *pRenderView->pRenderData;
  auto &               terrains   = renderData.renderables<ProceduralPlanetRenderable>();
  for (auto const & [transform, planet] : pLevel->getView<components::Transform, components::ProceduralPlanet>()) {
    ProceduralPlanetRenderable renderable;
    renderable.planetID    = pLevel->toEntity(&transform);
    renderable.transform   = transform.globalTransform(pLevel);
    renderable.seed        = planet.seed;
    renderable.minHeight   = planet.minHeight;
    renderable.maxHeight   = planet.maxHeight;
    renderable.scale       = planet.scale;
    renderable.radius      = planet.radius;
    renderable.octaves     = planet.octaves;
    renderable.persistance = planet.persistance;
    renderable.lacurnarity = planet.lacurnarity;
    terrains.pushBack(renderable);
  }
}
