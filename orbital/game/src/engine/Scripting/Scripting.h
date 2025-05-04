#pragma once

#include "Subsystem.h"
#include "scripting/WrenContext.h"

namespace components {
  struct Script {
    bfc::String componentName; ///< Typename of the script component

    bfc::scripting::wren::Value type;
    bfc::scripting::wren::Value instance;
  };
} // namespace components

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
