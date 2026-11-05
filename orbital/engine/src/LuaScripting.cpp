#include "LuaScripting.h"
#include "Assets/AssetLoadContext.h"
#include "Assets/AssetLoader.h"
#include "Assets/AssetManager.h"
#include "Application.h"
#include "Levels/Level.h"

namespace engine {
  bool ScriptAsset::load(sol::state * pLua, bfc::StringView src) {
    m_env   = { *pLua, sol::create};
    m_class = pLua->safe_script(std::string_view(src.begin(), src.length()), m_env);
    m_new   = m_class["new"];
    return m_new != nullptr;
  }

  sol::table ScriptAsset::instantiate() {
    return m_new();
  }

  class ScriptAssetLoader : public AssetLoader<ScriptAsset> {
  public:
    ScriptAssetLoader(LuaScripting * pScripting)
      : m_pScripting(pScripting) {}

    virtual bfc::Ref<ScriptAsset> load(bfc::URI const & uri, AssetLoadContext * pContext) const {
      auto *      pFS = pContext->getFileSystem();
      bfc::String sourceCode;
      if (!pFS->readText(uri, &sourceCode)) {
        return nullptr;
      }

      auto pScript = bfc::NewRef<ScriptAsset>();
      if (!pScript->load(&m_pScripting->lua, sourceCode)) {
        return nullptr;
      }

      return pScript;
    }

    virtual bool handles(bfc::URI const & uri, AssetManager const * pManager) const {
      return uri.path().extension().equals("lua", true);
    }

    LuaScripting * m_pScripting = nullptr;
  };

  LuaScripting::LuaScripting()
    : Subsystem(bfc::TypeID<LuaScripting>(), "Scripting") {}

  bool LuaScripting::init(Application * pApp) {
    reload();

    auto pAssetManager = pApp->findSubsystem<AssetManager>();
    pAssetManager->registerLoader("core.scripts.lua", bfc::NewRef<ScriptAssetLoader>(this));

    registerComponentType<components::LuaScripts>("core.scripts");
    registerLevelSystem<LuaScriptRunner>();
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
    lua.open_libraries(sol::lib::base);

    sol::table engine = lua.create_table("Engine");

    getEvents()->broadcast(events::OnReloadScripts{this});

    auto pAssetManager = getApp()->findSubsystem<AssetManager>();
    for (AssetHandle & script : pAssetManager->findHandles<ScriptAsset>()) {
      pAssetManager->reload(script);
    }
  }

  // LuaScriptRunner::LuaScriptRunner(bfc::Ref<bfc::EventListener> const & pEvents)
  //   : m_pEvents(pEvents) {
  //   m_pEvents->on<>();
  // }

  void LuaScriptRunner::update(Level * pLevel, bfc::Timestamp dt) {
    for (auto & [script] : pLevel->getView<components::LuaScripts>()) {
      script.instantiate();

      for (auto & instance : script.instances())
        instance.update(pLevel, dt);
    }
  }

  void LuaScriptRunner::activate(Level * pLevel) {
    // maybe a good idea? need to re-init the environment if the lua context is reloaded.
    // pLevel->addData<sol::environment>(lua, sol::create);

    for (auto & [script] : pLevel->getView<components::LuaScripts>()) {
      script.instantiate();

      for (auto & instance : script.instances())
        instance.activate(pLevel);
    }
  }

  void LuaScriptRunner::deactivate(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::LuaScripts>())
      for (auto & instance : script.instances())
        instance.deactivate(pLevel);
  }

  void LuaScriptRunner::pause(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::LuaScripts>())
      for (auto & instance : script.instances())
        instance.pause(pLevel);
  }

  void LuaScriptRunner::stop(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::LuaScripts>())
      for (auto & instance : script.instances())
        instance.stop(pLevel);
  }

  void LuaScriptRunner::destroyed(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::LuaScripts>())
      for (auto & instance : script.instances())
        instance.destroyed(pLevel);
  }
} // namespace engine
