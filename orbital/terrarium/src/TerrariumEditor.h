#pragma once

#include "Subsystem.h"

class TerrariumEditor : public engine::Subsystem {
public:
  TerrariumEditor();

  virtual bool init(engine::Application * pApp);
};
