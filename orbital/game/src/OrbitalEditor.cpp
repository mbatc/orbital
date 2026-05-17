#include "OrbitalEditor.h"
#include "Editor/PlayerControlEditor.h"
#include "Editor/ProceduralTerrainEditor.h"
#include "Application.h"
#include "Input.h"

using namespace engine;
using namespace bfc;

OrbitalEditor::OrbitalEditor()
  : Subsystem(bfc::TypeID<OrbitalEditor>(), "OrbitalEditor") {}

bool OrbitalEditor::init(engine::Application * pApp) {
  BFC_UNUSED(pApp);

  Ref<LevelEditor> pLevelEditor = pApp->findSubsystem<LevelEditor>();
  pLevelEditor->addComponentEditor<VehicleCameraControllerEditor>();
  pLevelEditor->addComponentEditor<VehicleControllerEditor>();
  pLevelEditor->addComponentEditor<VehicleVelocityEditor>();

  pLevelEditor->addComponentEditor<ProceduralPlanetEditor>();
  return true;
}
