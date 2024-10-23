#pragma once

#include "Subsystem.h"

class OrbitalEditor : public engine::Subsystem {
public:
  OrbitalEditor();

  virtual bool init(engine::Application * pApp);
};
