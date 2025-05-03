#pragma once

#include "Subsystem.h"

namespace bfc {
  namespace scripting {
    class WrenContext;
  }
}

namespace engine {
  class Scripting : public Subsystem {
  public:
    // struct {
    //   bfc::Setting<bfc::Vector<bfc::URI>> scriptPaths;
    // } settings;

    Scripting();
    // ~Scripting();

    virtual bool init(Application * pApp) override;
    virtual void shutdown() override;

    /// Reload the scripting context.
    void reload();

  private:
    bfc::Ref<bfc::scripting::WrenContext> m_pContext;
  };
}
