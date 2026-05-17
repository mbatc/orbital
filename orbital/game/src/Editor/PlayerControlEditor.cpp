#include "PlayerControlEditor.h"
#include "ui/Widgets.h"

using namespace bfc;
using namespace engine;

void VehicleCameraControllerEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID,
                                         VehicleCameraController * pComponent) {
  pEditor->drawEntitySelector("Target", &pComponent->target, pLevel.get());
}

void VehicleControllerEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID,
                                   VehicleController * pComponent) {
  pEditor->drawEntitySelector("Target", &pComponent->target, pLevel.get());
}

void VehicleVelocityEditor::draw(LevelEditor * pEditor, Ref<Level> const & pLevel, EntityID entityID,
                                 VehicleVelocity * pComponent) {
  ui::Input("Velocity", &pComponent->velocity);
}
