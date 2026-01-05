#include "ProceduralTerrainEditor.h"
#include "ui/Widgets.h"

using namespace bfc;
using namespace engine;

void ProceduralPlanetEditor::draw(engine::LevelEditor * pEditor, bfc::Ref<engine::Level> const & pLevel, engine::EntityID entityID,
                                  components::ProceduralPlanet * pComponent) {
  int64_t tmp = pComponent->seed;
  ui::Slider<int64_t>("Seed", &tmp, 0, std::numeric_limits<uint32_t>::max());
  pComponent->seed = (uint32_t)tmp;
  ui::Slider<float>   ("Scale",      &pComponent->scale, 0, 10);
  ui::Slider<float>   ("Min Height", &pComponent->minHeight, -1000, 1000);
  ui::Slider<float>   ("Max Height", &pComponent->maxHeight, -1000, 1000);
  ui::Slider<double>  ("Radius",     &pComponent->radius, 0, 100);
}
