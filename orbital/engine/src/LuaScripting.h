#pragma once

#include "subsystem.h"
#include "../../../vendor/sol2/include/sol/sol.hpp"
#include "Levels/LevelSystem.h"
#include "Assets/AssetManager.h"
#include "Levels/Level.h"

namespace engine {
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
    bfc::Ref<VirtualFileSystem> m_pFileSystem = nullptr;
  };
} // namespace engine

// sol extensions for engine types
namespace sol {
  int sol_lua_push(sol::types<bfc::String>, lua_State * L, const bfc::String & str);

  bfc::String sol_lua_get(sol::types<bfc::String>, lua_State * L, int index, sol::stack::record & tracking);

  template<typename Handler>
  bool sol_lua_check(types<bfc::String>, lua_State * L, int index, Handler && handler, sol::stack::record & tracking) {
    int  absolute_index = lua_absindex(L, index);
    bool success        = sol::stack::check<const char *>(L, absolute_index, std::forward<Handler>(handler));
    tracking.use(1);
    return success;
  }

  template<>
  struct lua_type_of<bfc::String> : std::integral_constant<type, type::string> {};

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
  } // namespace stack

  template<typename Value>
  struct usertype_container<bfc::Vector<Value>> : container_detail::usertype_container_default<bfc::Vector<Value>> {
    using Vec = bfc::Vector<Value>;
    // Only override the problematic method
    static int find(lua_State * L) {
      Vec &    self = sol::stack::get<Vec &>(L, 1);
      Value && key  = sol::stack::get<Value>(L, 2); // Match your find() parameter type
      int64_t  idx  = self.find(key);
      if (idx < -1) {
        return sol::stack::push(L, sol::lua_nil);
      }
      return sol::stack::push(L, idx + 1);
    }
  };
} // namespace sol
