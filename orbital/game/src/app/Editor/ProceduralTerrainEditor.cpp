#include "ProceduralTerrainEditor.h"
#include "ui/Widgets.h"

using namespace bfc;
using namespace engine;

void ProceduralTerrainEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID, ProceduralTerrain * pComponent) {
  if (ImGui::CollapsingHeader("Height Map")) {
    uint64_t seed = pComponent->heightMap.seed;
    ui::Slider<uint64_t>("Seed", &seed, 0, std::numeric_limits<uint32_t>::max());
    pComponent->heightMap.seed = (uint32_t)seed;

    ui::Slider<uint32_t>("Frequency", &pComponent->heightMap.frequency, 0, 64);
    ui::Slider<uint32_t>("Octaves", &pComponent->heightMap.octaves, 0, 32);
    ui::Slider<float>("Lacunarity", &pComponent->heightMap.lacunarity, 1, 16);
    ui::Slider<float>("Persistence", &pComponent->heightMap.persistence, 0, 1);
  }
}
