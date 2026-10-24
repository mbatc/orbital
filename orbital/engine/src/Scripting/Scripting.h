#pragma once

#include "../subsystem.h"
#include "../../../../vendor/sol2/include/sol/sol.hpp"

namespace engine {
  class ScriptAsset {
  public:
    bfc::String source;
  };

  class Scripting : public Subsystem {
  public:
    virtual bool init(Application * pApp);
    virtual void shutdown();
    virtual bool parseCommandLine(int argc, char ** argv);
    virtual void loop(Application * pApp);

    /// Reload the scripting context
    void reload();

    // template<typename L>
    // void exportFunctio() {
    // 
    // }

    template<typename C>
    void exportType() {
      bfc::reflect<C>();
    }

    auto run(ScriptAsset const & asset) {
      lua.script(asset.source.c_str());
    }

  private:
    sol::state lua;
  };
}
