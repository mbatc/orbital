#pragma once

#include "core/typeindex.h"
#include "core/String.h"
#include "platform/Events.h"

namespace engine {
  class Application;
  class Subsystem {
    friend Application;
  public:
    Subsystem(bfc::type_index const & type, bfc::String const & name)
      : m_events(name)
      , type(type) {}

    const bfc::type_index type;
    const bfc::String     name;

    virtual bool init(Application * pApp) { BFC_UNUSED(pApp); return true; }
    virtual void shutdown() {}

    virtual bool parseCommandLine(int argc, char** argv) { BFC_UNUSED(argc, argv); return true; }
    virtual void loop(Application * pApp) {}

    bfc::Events * getEvents();
    bfc::Events const * getEvents() const;

    Application * getApp() const;

  private:
    Application * m_pApp = nullptr;
    bfc::Events m_events;
  };
}
