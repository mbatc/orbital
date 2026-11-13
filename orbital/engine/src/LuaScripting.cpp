#include "LuaScripting.h"
#include "Assets/AssetLoadContext.h"
#include "Assets/AssetLoader.h"
#include "Assets/AssetManager.h"
#include "Application.h"
#include "Levels/Level.h"
#include "Util/log.h"

#include "Behaviours.h"

namespace engine {
  namespace impl {
    inline int luaExceptionHandler(lua_State *L, sol::optional<const std::exception &>, sol::string_view what) {
      BFC_LOG_ERROR("lua", "An exception occurred: %.*s", (int)what.size(), what.data());
      lua_pushlstring(L, what.data(), what.size());
      return 1;
    }
  } // namespace impl

  class LuaBehaviourInstance : public BehaviourInstance {
  public:
    LuaBehaviourInstance(BehaviourClass * pClass, sol::table instance)
      : m_pClass(pClass)
      , m_instance(instance) 
      , m_update(instance["update"])
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
    void activate(engine::Level * pLevel) override {
      if (m_activate)
        m_activate(m_instance, pLevel);
    }

    /// Call the scripts deactivate method
    void deactivate(engine::Level * pLevel) override {
      if (m_deactivate)
        m_deactivate(m_instance, pLevel);
    }

    /// Call the scripts update method
    void update(engine::Level * pLevel, bfc::Timestamp dt) override {
      if (m_update)
        m_update(m_instance, pLevel, dt.secs());
    }

    /// Call the scripts stop method
    void stop(engine::Level * pLevel) override {
      if (m_stop)
        m_stop(m_instance, pLevel);
    }

    /// Call the scripts pause method
    void pause(engine::Level * pLevel) override {
      if (m_pause)
        m_pause(m_instance, pLevel);
    }

    /// Call the scripts destroy method
    void destroyed(engine::Level * pLevel) override {
      if (m_destroyed)
        m_destroyed(m_instance, pLevel);
    }

    /// Get the class that created this instance
    virtual BehaviourClass* getClass() const override {
      return m_pClass;
    }

    sol::protected_function m_update;
    sol::protected_function m_activate;
    sol::protected_function m_deactivate;
    sol::protected_function m_pause;
    sol::protected_function m_stop;
    sol::protected_function m_destroyed;

    sol::table       m_instance;
    BehaviourClass * m_pClass = nullptr;
  };

  class LuaBehaviourClass : public BehaviourClass {
  public:
    LuaBehaviourClass(sol::state * pLua, sol::table classTable, sol::protected_function bindToEntity) {
      m_bindToEntity = bindToEntity;
      m_class        = classTable;
      m_new          = m_class.as<sol::protected_function>();
    }

    /// Create an instance of this behaviour class.
    virtual bfc::Ref<BehaviourInstance> instantiate(engine::Level * pLevel, engine::EntityID entity) {
      auto result = m_new();
      if (!result.valid()) {
        sol::error err = result;
        BFC_LOG_ERROR("lua", "Function error: %s", err.what());
        return nullptr;
      }

      auto table = result.get<sol::table>();
      if (!m_bindToEntity(table, pLevel, entity))
        return nullptr;

      return bfc::NewRef<LuaBehaviourInstance>(this, table);
    }

    /// Serialize an instance of this behaviour class.
    virtual bfc::SerializedObject serialize(BehaviourInstance * pSerialize) {
      return bfc::SerializedObject::Empty();
    }

    /// Deserialize an instance of this behaviour class.
    virtual bfc::Ref<BehaviourInstance> deserialize(bfc::SerializedObject const & serialization) {
      return nullptr;
    }

    sol::table              m_class;
    sol::protected_function m_new;
    sol::protected_function m_bindToEntity;
  };

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
    lua = {};
    lua.add_package_loader([this](bfc::String const & str) -> sol::object {
      bfc::String content;
      if (!m_pFileSystem->readText(bfc::URI(str), &content))
        return sol::make_object(lua, (std::string)bfc::String::format("\n\tUnable to read module: %s", str));
      return lua.load_buffer(content.begin(), content.length(), str.begin()).get<sol::object>();
    });
    lua.set_exception_handler(impl::luaExceptionHandler);
    lua.open_libraries(
      sol::lib::base,
      sol::lib::table,
      sol::lib::package,
      sol::lib::coroutine,
      sol::lib::io,
      sol::lib::debug,
      sol::lib::math
    );

    bfc::String entryScript;
    if (!m_pFileSystem->readText("file:///engine:lua/entry.lua", &entryScript)) {
      return;
    }
    // Run core script
    lua.script({entryScript.begin(), (size_t)entryScript.length()});

    sol::table engine = lua["Engine"];
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

    detail["addBehaviour"] = [=](bfc::String identifier, sol::table luaClass) {
      return BehaviourClass::Registry::add(identifier,
                                           bfc::NewRef<LuaBehaviourClass>(&lua, luaClass));
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
