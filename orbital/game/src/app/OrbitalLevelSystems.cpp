#include "OrbitalLevelSystems.h"
#include "Systems/PlayerControl.h"
#include "Systems/ProceduralTerrain.h"
#include "Rendering/Rendering.h"
#include "Application.h"
#include "Input.h"

OrbitalGameSystems::OrbitalGameSystems()
  : Subsystem(bfc::TypeID<OrbitalGameSystems>(), "OrbitalGameSystems") {
}

bool OrbitalGameSystems::init(engine::Application * pApp) {
  BFC_UNUSED(pApp);

  registerComponents();
  registerSystems(pApp);
  return true;
}

void OrbitalGameSystems::registerComponents() {
  engine::registerComponentType<VehicleController>("vehicle-controller");
  engine::registerComponentType<VehicleCameraController>("vehicle-camera-controller");
  engine::registerComponentType<components::ProceduralPlanet>("procedural-planet");
}

void OrbitalGameSystems::registerSystems(engine::Application * pApp) {
  engine::registerLevelSystem<PlayerControlSystem>(pApp->findSubsystem<engine::Input>());
  engine::registerLevelSystem<ProceduralTerrainSystem>(pApp->findSubsystem<engine::Rendering>(), pApp->findSubsystem<engine::AssetManager>());
}
