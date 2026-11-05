#pragma once

#include "subsystem.h"
#include "../../../vendor/sol2/include/sol/sol.hpp"
#include "Levels/LevelSystem.h"
#include "Assets/AssetManager.h"
#include "Levels/Level.h"

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
  class LuaScripting;

  namespace events {
    struct OnReloadScripts {
      LuaScripting * pScriptManager;
    };
  }

  enum ScriptHandle : int64_t {};
  
  class ScriptAsset {
  public:
    bool       load(sol::state * pLua, bfc::StringView src);
    sol::table instantiate();

  private:
    sol::environment        m_env;
    sol::table              m_class;
    sol::protected_function m_new;
  };
}

namespace components {
  class LuaScripts {
  public:
    bool has(engine::Asset<engine::ScriptAsset> const & pScript) const {
      return m_scripts.contains(pScript);
    }

    bool add(engine::Asset<engine::ScriptAsset> const& pScript) {
      if (has(pScript))
        return false;
      m_scripts.pushBack(pScript);
      return true;
    }

    bool remove(engine::Asset<engine::ScriptAsset> const & pScript) {
      return m_scripts.eraseValue(pScript);
    }

    class Instance {
    public:
      Instance(sol::table instance)
        : m_update(instance["update"])
        , m_activate(instance["activate"])
        , m_deactivate(instance["deactivate"])
        , m_pause(instance["pause"])
        , m_stop(instance["stop"])
        , m_destroyed(instance["destroyed"])
      {}

      /// Call a method on this script
      template<typename... Args>
      sol::protected_function_result call(bfc::StringView const & name, Args &&... args) const {
        if (!m_instance.has_value())
          return nullptr;

        sol::protected_function func = m_instance.value()[name];
        return func(m_instance.value(), std::forward<Args>(args)...);
      }

      template<typename... Args>
      sol::protected_function_result callStatic(bfc::StringView const & name, Args &&... args) const {
        if (!m_instance.has_value())
          return nullptr;

        sol::protected_function func = m_instance.value()[name];
        return func(std::forward<Args>(args)...);
      }

      /// Call the scripts activate method
      void activate(engine::Level * pLevel) const {
        if (m_activate)
          m_activate(pLevel);
      }

      /// Call the scripts activate method
      void deactivate(engine::Level * pLevel) const {
        if (m_deactivate)
          m_deactivate(pLevel);
      }

      /// Call the scripts update method
      void update(engine::Level * pLevel, bfc::Timestamp dt) const {
        if (m_update)
          m_update(pLevel, dt.secs());
      }

      /// Call the scripts stop method
      void stop(engine::Level * pLevel) const {
        if (m_stop)
          m_stop(pLevel);
      }

      /// Call the scripts pause method
      void pause(engine::Level * pLevel) const {
        if (m_pause)
          m_pause(pLevel);
      }

      /// Call the scripts destroy method
      void destroyed(engine::Level * pLevel) const {
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
    };

    bfc::Vector<engine::Asset<engine::ScriptAsset>> const & scripts() const {
      return m_scripts;
    }

    bfc::Vector<Instance> const & instances() const {
      return m_instances;
    }

    void instantiate() {
      for (int64_t i = m_instances.size(); i < m_scripts.size(); ++i) {
        m_instances.pushBack(m_scripts[i]->instantiate());
      }
    }

  private:
    bfc::Vector<engine::Asset<engine::ScriptAsset>> m_scripts;
    bfc::Vector<Instance>                           m_instances;
  };
}

namespace engine {
  class LuaScriptRunner
    : public ILevelDestroyed
    , public ILevelUpdate
    , public ILevelActivate
    , public ILevelDeactivate
    , public ILevelPause
    , public ILevelStop {
  public:
  //   LuaScriptRunner(bfc::Ref<bfc::EventListener> const & pEvents);
  // private:
  //   bfc::Ref<bfc::EventListener> m_pEvents;
  // public:

    void destroyed(Level * pLevel) override;
    void update(Level * pLevel, bfc::Timestamp dt) override;
    void activate(Level * pLevel) override;
    void deactivate(Level * pLevel) override;
    void pause(Level * pLevel) override;
    void stop(Level * pLevel) override;
  };

  class LuaScripting : public Subsystem {
  public:
    LuaScripting();

    virtual bool init(Application * pApp);
    virtual void shutdown();
    virtual bool parseCommandLine(int argc, char ** argv);
    virtual void loop(Application * pApp);

    sol::protected_function_result run(bfc::StringView const & script, sol::environment env);

    /// Reload the scripting context
    void reload();

    sol::state lua;
  private:
  };
} // namespace engine

namespace bfc {
  template<>
  struct Serializer<components::LuaScripts> {
    template<typename Context>
    static SerializedObject write(components::LuaScripts const & o, Context const & ctx) {
      return SerializedObject::MakeMap({
        { "scripts", serialize(o.scripts(), ctx) },
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, components::LuaScripts & o, Context const & ctx) {
      mem::construct(&o);

      Uninitialized<bfc::Vector<engine::Asset<engine::ScriptAsset>>> scripts;
      if (!s.get("scripts").read(scripts.get(), ctx)) {
        return false;
      }
      
      for (const engine::Asset<engine::ScriptAsset> & script : scripts.get()) {
        o.add(script);
      }

      scripts.destruct();
      return true;
    }
  };
} // namespace bfc
