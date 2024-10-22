#pragma once

#include "Subsystem.h"

class OrbitalGameSystems : public engine::Subsystem {
public:
  OrbitalGameSystems();

  virtual bool init(engine::Application * pApp);

private:
  void registerComponents();
  void registerSystems(engine::Application * pApp);
};
