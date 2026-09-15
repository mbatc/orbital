#pragma once

#include "Subsystem.h"

class TerrariumGameSystems : public engine::Subsystem {
public:
  TerrariumGameSystems();

  virtual bool init(engine::Application * pApp);

private:
  void registerComponents();
  void registerSystems(engine::Application * pApp);
};
