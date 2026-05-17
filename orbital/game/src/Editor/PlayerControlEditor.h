#pragma once

#include "Editor/LevelEditor.h"
#include "Systems/PlayerControl.h"

class VehicleCameraControllerEditor : public engine::LevelEditor::ComponentEditor<VehicleCameraController> {
public:
  virtual void draw(engine::LevelEditor * pEditor, bfc::Ref<engine::Level> const & pLevel, engine::EntityID entityID,
                    VehicleCameraController * pComponent) override;
};

class VehicleControllerEditor : public engine::LevelEditor::ComponentEditor<VehicleController> {
public:
  virtual void draw(engine::LevelEditor * pEditor, bfc::Ref<engine::Level> const & pLevel, engine::EntityID entityID, VehicleController * pComponent) override;
};

class VehicleVelocityEditor : public engine::LevelEditor::ComponentEditor<VehicleVelocity> {
public:
  virtual void draw(engine::LevelEditor * pEditor, bfc::Ref<engine::Level> const & pLevel, engine::EntityID entityID, VehicleVelocity * pComponent) override;
};
