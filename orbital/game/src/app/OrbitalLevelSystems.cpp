#include "OrbitalLevelSystems.h"
#include "Systems/PlayerControl.h"
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
}

void OrbitalGameSystems::registerSystems(engine::Application * pApp) {
  engine::registerLevelSystem<PlayerControlSystem>(pApp->findSubsystem<engine::Input>());
}
