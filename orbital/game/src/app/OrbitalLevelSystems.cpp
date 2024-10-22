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
  engine::registerComponentType<VehicleControls>("vehical-controls");
  engine::registerComponentType<VehicleCameraControls>("vehical-camera-controls");
}

void OrbitalGameSystems::registerSystems(engine::Application * pApp) {
  engine::registerLevelSystem<PlayerControlSystem>(pApp->findSubsystem<engine::Input>());
}
