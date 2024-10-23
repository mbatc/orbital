#include "PlayerControlEditor.h"
#include "ui/Widgets.h"

using namespace bfc;
using namespace engine;

void VehicleCameraControllerEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID,
                                         VehicleCameraController * pComponent) {
  pEditor->drawEntitySelector("Follow", &pComponent->follow, pLevel.get());
}

void VehicleControllerEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID,
                                   VehicleController * pComponent) {}

void VehicleVelocityEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID,
                                 VehicleVelocity * pComponent) {
  ui::Input("Velocity", &pComponent->velocity);
}
