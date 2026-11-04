#pragma once

#include "../subsystem.h"
#include "../../../../vendor/sol2/include/sol/sol.hpp"
#include "Levels/LevelSystem.h"
#include "Assets/AssetManager.h"

namespace sol {
  namespace stack {
    template<>
    struct unqualified_pusher<bfc::String> {
      static int push(lua_State * L, const bfc::String & sv) {
        return stack::push(L, sv.data(), sv.length());
      }

      static int push(lua_State * L, const bfc::String & sv, std::size_t n) {
        return stack::push(L, sv.data(), n);
      }
    };

    template<>
    struct unqualified_pusher<bfc::StringView> {
      static int push(lua_State * L, const bfc::StringView & sv) {
        return stack::push(L, sv.data(), sv.length());
      }

      static int push(lua_State * L, const bfc::StringView & sv, std::size_t n) {
        return stack::push(L, sv.data(), n);
      }
    };
  }
}

namespace engine {
  class Scripting;

  namespace events {
    struct OnReloadScripts {
      Scripting * pScriptManager;
    };
  }

  enum ScriptHandle : int64_t {};
  
  using ScriptInstance = sol::table;

  class ScriptAsset {
  public:
    ScriptInstance handle;
    ScriptInstance instantiate();
  };

}

namespace components {
  struct Script {
  public:
    engine::Asset<engine::ScriptAsset> pAsset;

    /// Instantiate this script
    bool instantiate() {
      if (pAsset == nullptr)
        return false;
      m_instance   = pAsset->instantiate();
      m_update     = m_instance.value()["update"];
      m_activate   = m_instance.value()["activate"];
      m_deactivate = m_instance.value()["deactivate"];
      m_pause      = m_instance.value()["pause"];
      m_stop       = m_instance.value()["stop"];
      m_destroyed  = m_instance.value()["destroyed"];
      return true;
    }

    /// Call a method on this script
    template<typename... Args>
    sol::protected_function_result call(bfc::StringView const& name, Args &&... args) {
      if (!m_instance.has_value())
        return nullptr;
      
      sol::protected_function func = m_instance.value()[name];
      return func(m_instance.value(), std::forward<Args>(args)...);
    }

    template<typename... Args>
    sol::protected_function_result callStatic(bfc::StringView const & name, Args &&... args) {
      if (!m_instance.has_value())
        return nullptr;

      sol::protected_function func = m_instance.value()[name];
      return func(std::forward<Args>(args)...);
    }

    /// Call the scripts activate method
    void activate(engine::Level* pLevel) {
      if (m_activate)
        m_activate(pLevel);
    }

    /// Call the scripts activate method
    void deactivate(engine::Level * pLevel) {
      if (m_deactivate)
        m_deactivate(pLevel);
    }

    /// Call the scripts update method
    void update(engine::Level * pLevel, bfc::Timestamp dt) {
      if (m_update)
        m_update(pLevel, dt.secs());
    }

    /// Call the scripts stop method
    void stop(engine::Level * pLevel) {
      if (m_stop)
        m_stop(pLevel);
    }

    /// Call the scripts pause method
    void pause(engine::Level * pLevel) {
      if (m_pause)
        m_pause(pLevel);
    }

    /// Call the scripts destroy method
    void destroy(engine::Level * pLevel) {
      if (m_destroyed)
        m_destroyed(pLevel);
    }

  private:
    sol::protected_function m_update;
    sol::protected_function m_activate;
    sol::protected_function m_deactivate;
    sol::protected_function m_pause;
    sol::protected_function m_stop;
    sol::protected_function m_destroyed;

    std::optional<engine::ScriptInstance> m_instance;
  };
}

namespace engine {
  class ScriptRunner
    : public ILevelDestroyed
    , public ILevelUpdate
    , public ILevelActivate
    , public ILevelDeactivate
    , public ILevelPause
    , public ILevelStop {
  public:
    // Inherited via ILevelDestroyed
    void destroyed(Level * pLevel) override;

    // Inherited via ILevelUpdate
    void update(Level * pLevel, bfc::Timestamp dt) override;

    // Inherited via ILevelActivate
    void activate(Level * pLevel) override;

    // Inherited via ILevelDeactivate
    void deactivate(Level * pLevel) override;

    // Inherited via ILevelPause
    void pause(Level * pLevel) override;

    // Inherited via ILevelStop
    void stop(Level * pLevel) override;
  };

  class Scripting : public Subsystem {
  public:
    Scripting();

    virtual bool init(Application * pApp);
    virtual void shutdown();
    virtual bool parseCommandLine(int argc, char ** argv);
    virtual void loop(Application * pApp);

    /// Reload the scripting context
    void reload();

    // template<typename L>
    // void exportFunction() {
    // 
    // }

    void exportVec3() {
      sol::table engine = lua["Engine"];
      // engine.new_usertype<bfc::Vec3>("Vec3");
    }

    template<typename C>
    void exportType(bfc::StringView ns = "Engine") {
      bfc::reflect<C>();
    }

    auto run(ScriptAsset const & asset) {
      lua.script(asset.source.c_str());
    }

    ScriptHandle loadScript(bfc::StringView const & source);
    void         unloadScript(ScriptHandle handle);

  private:
    sol::state lua;
  };
}
