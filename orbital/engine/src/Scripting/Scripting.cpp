#include "Scripting.h"
#include "Application.h"
#include "../Assets/AssetManager.h"
#include "../Assets/AssetLoader.h"
#include "../Assets/AssetLoadContext.h"
#include "Levels/Level.h"

namespace engine {
  class ScriptAssetLoader : public AssetLoader<ScriptAsset> {
    ScriptAssetLoader(Scripting * pScripting)
      : m_pScripting(pScripting) {}

    virtual bfc::Ref<ScriptAsset> load(bfc::URI const & uri, AssetLoadContext * pContext) const {
      return m_pScripting->loadScript(pContext->getFileSystem(), uri);
      auto *pFS = pContext->getFileSystem();
      bfc::String sourceCode;
      if (!pFS->readText(uri, &sourceCode)) {
        return nullptr;
      }

      auto pScript = bfc::NewRef<ScriptAsset>();
      pScript->source = sourceCode;
      return pScript;
    }

    virtual bool handles(bfc::URI const& uri, AssetManager const * pManager) const {
      return uri.path().extension().equals("lua", true);
    }

    Scripting * m_pScripting = nullptr;
  };

  Scripting::Scripting()
    : Subsystem(bfc::TypeID<Scripting>(), "Scripting")
  {}

  bool Scripting::init(Application * pApp) {
    reload();

    auto pAssetManager = pApp->findSubsystem<AssetManager>();
    pAssetManager->registerLoader("core.scripts.lua", bfc::NewRef<ScriptAssetLoader>());
    auto script = pAssetManager->load<ScriptAsset>("file:///game:/scripts/hello_scripts.lua");

    sol::environment env(lua, sol::create);
    sol::table       behaviour = lua.safe_script(std::string_view(script->source.begin(), script->source.length()), env);
    sol::table       behaviourInstance = behaviour["new"]();
    int              result            = behaviourInstance["update"](behaviourInstance);

    BFC_UNUSED(pApp, result);
    return true;
  }

  void Scripting::shutdown() {
    lua = {};
  }

  bool Scripting::parseCommandLine(int argc, char ** argv) {
    BFC_UNUSED(argc, argv);
    return true;
  }

  void Scripting::loop(Application * pApp) {

  }

  void Scripting::reload() {
    lua = {};
    lua.open_libraries(sol::lib::base);

    sol::table engine = lua.create_table("Engine");

    getEvents()->broadcast(events::OnReloadScripts{ this });
  }

  void ScriptRunner::destroyed(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Script>())
      script.destroy(pLevel);
  }

  void ScriptRunner::update(Level * pLevel, bfc::Timestamp dt) {
    for (auto & [script] : pLevel->getView<components::Script>())
      script.update(pLevel, dt);
  }

  void ScriptRunner::activate(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Script>())
      script.activate(pLevel);
  }

  void ScriptRunner::deactivate(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Script>())
      script.deactivate(pLevel);
  }

  void ScriptRunner::pause(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Script>())
      script.pause(pLevel);
  }

  void ScriptRunner::stop(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Script>())
      script.stop(pLevel);
  }
} // namespace engine
