#include "LuaScripting.h"
#include "Assets/AssetLoadContext.h"
#include "Assets/AssetLoader.h"
#include "Assets/AssetManager.h"
#include "Application.h"
#include "Levels/Level.h"
#include "Util/log.h"

#include "Behaviours.h"
#include "LuaBehaviours.h"

namespace engine {
  namespace impl {
    inline int luaExceptionHandler(lua_State *L, sol::optional<const std::exception &>, sol::string_view what) {
      BFC_LOG_ERROR("lua", "An exception occurred: %.*s", (int)what.size(), what.data());
      lua_pushlstring(L, what.data(), what.size());
      return 1;
    }
  } // namespace impl

  LuaScripting::LuaScripting()
    : Subsystem(bfc::TypeID<LuaScripting>(), "Scripting") {}

  bool LuaScripting::init(Application * pApp) {
    m_pFileSystem = pApp->findSubsystem<VirtualFileSystem>();
    reload();
    return true;
  }

  sol::protected_function_result LuaScripting::run(bfc::StringView const & script, sol::environment env) {
    return lua.safe_script(std::string_view(script.begin(), script.length()), env);
  }

  void LuaScripting::shutdown() {
    if (lua["Engine"].valid())
      lua["Engine"]["shutdown"]();
    lua = {};
  }

  bool LuaScripting::parseCommandLine(int argc, char ** argv) {
    BFC_UNUSED(argc, argv);
    return true;
  }

  void LuaScripting::loop(Application * pApp) {
    BFC_UNUSED(pApp);
  }

  void LuaScripting::reload() {
    if (lua["Engine"].valid())
      lua["Engine"]["shutdown"]();
    lua = {};
    lua.add_package_loader([this](bfc::String const & str) -> sol::object {
      bfc::String content;
      if (!m_pFileSystem->readText(bfc::URI(str), &content))
        return sol::make_object(lua, (std::string)bfc::String::format("\n\tUnable to read module: %s", str));
      return lua.load_buffer(content.begin(), content.length(), str.begin()).get<sol::object>();
    });
    lua.set_exception_handler(impl::luaExceptionHandler);
    lua.open_libraries(sol::lib::base, sol::lib::table, sol::lib::package, sol::lib::coroutine, sol::lib::io,
                       sol::lib::debug, sol::lib::math);

    // lua.new_usertype<bfc::SerializedObject>("SerializedObject",
    //   "new", &sol::no_constructor,
    //   "MakeMap", &bfc::SerializedObject::MakeMap,
    //   "MakeArray", &bfc::SerializedObject::MakeArray,
    //   "MakeFloat", &bfc::SerializedObject::MakeFloat,
    //   "MakeInt", &bfc::SerializedObject::MakeInt,
    //   "MakeText", &bfc::SerializedObject::MakeText,
    //   "asArray", sol::resolve<bfc::Vector<bfc::SerializedObject> &()>(&bfc::SerializedObject::asArray),
    //   "asMap", sol::resolve<bfc::Map<bfc::String, bfc::SerializedObject> &()>(&bfc::SerializedObject::asMap),
    //   "asInt", &bfc::SerializedObject::asInt,
    //   "asFloat", &bfc::SerializedObject::asFloat,
    //   "asText", &bfc::SerializedObject::asText,
    //   "add", &bfc::SerializedObject::add,
    //   "pushFront", &bfc::SerializedObject::pushFront,
    //   "pushBack", &bfc::SerializedObject::pushBack,
    //   "pushBack", &bfc::SerializedObject::pushBack
    // );

    bfc::URI    entryScriptUri = "file:///engine:lua/entry.lua";
    bfc::String entryScript;
    if (!m_pFileSystem->readText(entryScriptUri, &entryScript)) {
      return;
    }

    // Run core script
    lua.script({entryScript.begin(), (size_t)entryScript.length()}, entryScriptUri.c_str());

    // Set up remaining bindings
    sol::table engine = lua[sol::create_if_nil]["Engine"];
    sol::table detail = engine["detail"];
    engine["resolveModuleURI"] = [=](bfc::String name) {
      if (name.find("://") != bfc::npos)
        return m_pFileSystem->resolveUri(bfc::URI(name)).str();
      else
        return m_pFileSystem->resolveUri(bfc::URI::File(name.replace(".", "/"))).path().operator bfc::String();
    };

    engine["resolveRelativeReference"] = [=](bfc::String base, bfc::String ref) {
      if (ref.find("://") != bfc::npos)
        return m_pFileSystem->resolveUri(bfc::URI(ref)).str();
      else
        return bfc::URI::File(bfc::Filename::parent(base)).resolveRelativeReference(ref).path().operator bfc::String();
    };

    auto pBehaviourContext = bfc::NewRef<LuaBehaviourClass::Shared>();
    pBehaviourContext->bind      = engine["bindEntity"].get<sol::protected_function>();
    pBehaviourContext->unbind    = engine["unbindEntity"].get<sol::protected_function>();
    pBehaviourContext->nullValue = engine["Null"].get<sol::object>();

    detail["addBehaviour"] = [=](bfc::String identifier, sol::table luaClass) {
      return BehaviourClass::Registry::add(identifier, bfc::NewRef<LuaBehaviourClass>(&lua, luaClass, pBehaviourContext));
    };

    detail["removeBehaviour"] = [=](bfc::String identifier) { return BehaviourClass::Registry::remove(identifier); };

    // Call lua entry point
    engine["startup"]();
  }
} // namespace engine

namespace sol {
  int sol_lua_push(sol::types<bfc::String>, lua_State * L, const bfc::String & str) {
    lua_pushlstring(L, str.data(), str.length());
    return 1;
  }

  bfc::String sol_lua_get(sol::types<bfc::String>, lua_State * L, int index, sol::stack::record & tracking) {
    int          absolute_index = lua_absindex(L, index);
    std::size_t  len            = 0;
    const char * str            = lua_tolstring(L, absolute_index, &len);
    tracking.use(1);
    return bfc::String(str, len);
  }
} // namespace sol
