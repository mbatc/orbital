#include "ProceduralTerrain.h"

#include "Rendering/RenderScene.h"
#include "Rendering/RenderData.h"
#include "Rendering/Renderables.h"

#include "geometry/Box.h"
#include "Assets/AssetManager.h"
#include "mesh/Mesh.h"

struct ProceduralTerrainShaderInput {
  uint32_t seed        = 0;
  uint32_t frequency   = 1;
  uint32_t octaves     = 6;
  float    persistence = 0.5f;
  float    lacunarity  = 2.0f;
  float    padding[3];
};

ProceduralTerrainSystem::ProceduralTerrainSystem(bfc::Ref<engine::AssetManager> pManager)
  : m_quad  (pManager.get(), bfc::URI::File("engine:models/primitives/plane.obj"))
  , m_shader(pManager.get(), bfc::URI::File("engine:shaders/terrain/procedural-terrain.shader")) {}

void ProceduralTerrainSystem::update(engine::Level * pLevel, bfc::Timestamp dt) {
  BFC_UNUSED(pLevel, dt);
}

void ProceduralTerrainSystem::collectRenderData(engine::RenderView * pRenderView, engine::Level const * pLevel) {
  auto & meshes = pRenderView->pRenderData->renderables<engine::MeshRenderable>();

  for (auto & [transform, terrain] : pLevel->getView<components::Transform, ProceduralTerrain>()) {
    bfc::Mat4d modelMatrix  = transform.globalTransform(pLevel);
    bfc::Mat4d normalMatrix = glm::transpose(glm::inverse(modelMatrix));

    bfc::graphics::CommandList * pUploadCmds = pRenderView->pRenderData->getUploadCommandList();
    for (int64_t sm = 0; sm < m_quad->getSubmeshCount(); ++sm) {
      bfc::graphics::StructuredBuffer<ProceduralTerrainShaderInput> inputs(bfc::BufferUsageHint_Uniform);
      inputs.data.frequency   = terrain.heightMap.frequency;
      inputs.data.lacunarity  = terrain.heightMap.lacunarity;
      inputs.data.seed        = terrain.heightMap.seed;
      inputs.data.persistence = terrain.heightMap.persistence;
      inputs.data.octaves     = terrain.heightMap.octaves;
      inputs.upload(pUploadCmds);

      engine::MeshRenderable renderable(modelMatrix, normalMatrix, *m_quad, sm, nullptr, m_shader);

      renderable.primitiveType = bfc::PrimitiveType_Patches;
      renderable.materialBuffer = inputs;

      meshes.pushBack(renderable);
    }
  }
}
