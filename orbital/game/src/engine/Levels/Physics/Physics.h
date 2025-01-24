#pragma once

#include "Subsystem.h"

namespace engine {
  class Physics : public Subsystem {
  public:
    class World;

    Physics();

    virtual bool init(Application * pApp);
    virtual void loop(Application * pApp);
  };
}
