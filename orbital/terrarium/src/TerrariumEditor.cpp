#include "TerrariumEditor.h"
#include "Application.h"
#include "Input.h"

using namespace engine;
using namespace bfc;

TerrariumEditor::TerrariumEditor()
  : Subsystem(bfc::TypeID<TerrariumEditor>(), "OrbitalEditor") {}

bool TerrariumEditor::init(engine::Application * pApp) {
  BFC_UNUSED(pApp);

  // Ref<LevelEditor> pLevelEditor = pApp->findSubsystem<LevelEditor>();
  // pLevelEditor->addComponentEditor<VehicleCameraControllerEditor>();
  // pLevelEditor->addComponentEditor<VehicleControllerEditor>();
  // pLevelEditor->addComponentEditor<VehicleVelocityEditor>();
  // 
  // pLevelEditor->addComponentEditor<ProceduralPlanetEditor>();
  // pLevelEditor->addComponentEditor<PlanetAtmosphereEditor>();
  return true;
}
