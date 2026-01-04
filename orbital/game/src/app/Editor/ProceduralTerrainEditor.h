#pragma once

#include "Editor/LevelEditor.h"
#include "Systems/ProceduralTerrain.h"

class ProceduralPlanetEditor : public engine::LevelEditor::ComponentEditor<components::ProceduralPlanet> {
public:
  virtual void draw(engine::LevelEditor * pEditor, bfc::Ref<engine::Level> const & pLevel, engine::EntityID entityID,
                    components::ProceduralPlanet * pComponent) override;
};
