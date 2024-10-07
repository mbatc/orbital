#pragma once

#include "core/typeindex.h"
#include "core/String.h"
#include "platform/Events.h"

namespace engine {
  class Application;
  class Subsystem : bfc::Events {
  public:
    Subsystem(bfc::type_index const& type, bfc::String const & name)
      : bfc::Events(name)
      , type(type) {}

    const bfc::type_index type;
    const bfc::String     name;

    virtual bool init(Application * pApp) { BFC_UNUSED(pApp); return true; }
    virtual void shutdown() {}

    virtual bool parseCommandLine(int argc, char** argv) { BFC_UNUSED(argc, argv); return true; }
    virtual void loop() {}
  };
}
