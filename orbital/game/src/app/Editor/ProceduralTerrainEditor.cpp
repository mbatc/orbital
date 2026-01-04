#include "ProceduralTerrainEditor.h"
#include "ui/Widgets.h"

using namespace bfc;
using namespace engine;

void ProceduralPlanetEditor::draw(engine::LevelEditor * pEditor, bfc::Ref<engine::Level> const & pLevel, engine::EntityID entityID,
                                  components::ProceduralPlanet * pComponent) {
  ui::Input("Seed",       &pComponent->seed);
  ui::Input("Scale",      &pComponent->scale);
  ui::Input("Min Height", &pComponent->minHeight);
  ui::Input("Max Height", &pComponent->maxHeight);
}
