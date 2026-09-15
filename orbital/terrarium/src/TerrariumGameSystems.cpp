#include "TerrariumGameSystems.h"
#include "Rendering/Rendering.h"
#include "Application.h"
#include "Input.h"

TerrariumGameSystems::TerrariumGameSystems()
  : Subsystem(bfc::TypeID<TerrariumGameSystems>(), "OrbitalGameSystems") {
}

bool TerrariumGameSystems::init(engine::Application * pApp) {
  BFC_UNUSED(pApp);

  registerComponents();
  registerSystems(pApp);
  return true;
}

void TerrariumGameSystems::registerComponents() {
}

void TerrariumGameSystems::registerSystems(engine::Application * pApp) {
}
