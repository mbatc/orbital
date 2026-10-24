#include "Scripting.h"
#include "../Assets/AssetManager.h"
#include "../Assets/AssetLoader.h"
#include "../Assets/AssetLoadContext.h"

namespace engine {
  class ScriptAssetLoader : public AssetLoader<ScriptAsset> {
    virtual bfc::Ref<ScriptAsset> load(bfc::URI const & uri, AssetLoadContext * pContext) const {
      auto *pFS = pContext->getFileSystem();
      bfc::String sourceCode;
      if (!pFS->readText(uri, &sourceCode)) {
        return nullptr;
      }

      auto pScript = bfc::NewRef<ScriptAsset>();
      pScript->source   = sourceCode;
      return pScript;
    }

    virtual bool handles(bfc::URI const& uri, AssetManager const * pManager) const {
      return uri.path().extension().equals("lua", true);
    }
  };

  bool Scripting::init(Application * pApp) {
    BFC_UNUSED(pApp);
    return true;
  }

  void Scripting::shutdown() {

  }

  bool Scripting::parseCommandLine(int argc, char ** argv) {
    BFC_UNUSED(argc, argv);
    return true;
  }

  void Scripting::loop(Application * pApp) {

  }

  void Scripting::reload() {

  }
} // namespace engine
