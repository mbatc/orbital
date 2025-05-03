#pragma once

#include "Editor/LevelEditor.h"
#include "Systems/ProceduralTerrain.h"

class ProceduralTerrainEditor : public engine::LevelEditor::ComponentEditor<ProceduralTerrain> {
public:
  virtual void draw(engine::LevelEditor * pEditor, bfc::Ref<engine::Level> const & pLevel, engine::EntityID entityID, ProceduralTerrain * pComponent) override;
};
