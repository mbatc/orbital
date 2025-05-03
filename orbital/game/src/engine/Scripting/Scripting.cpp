#include "Scripting.h"
#include "Application.h"
#include "Input.h"

#include "util/Log.h"

#include "Assets/VirtualFileSystem.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetLoader.h"

#include "scripting/WrenContext.h"
#include "platform/OS.h"
#include "Levels/LevelSystem.h"

using namespace bfc;
using namespace scripting;

namespace engine {
  namespace impl {
    class ScriptComponent {
    public:
      wren::Value instance;
      wren::Value type;
    };

    class LevelComponentAdapter {
    public:
      LevelComponentAdapter(wren::Value const& systemType) {

      }
    };

    class LevelSystemAdapter
      : engine::ILevelUpdate
      , engine::ILevelStop
      , engine::ILevelPause
      , engine::ILevelPlay
      , engine::ILevelDeactivate
      , engine::ILevelActivate
      , engine::ILevelRenderDataCollector {
      LevelSystemAdapter(wren::Value const & systemType)
        : m_type(systemType) {}

      void bind() {
        for (auto& method : m_type.methods()) {
          switch (method.type) {
          case wren::MethodType_Constructor: bindConstructor(method); break;
          case wren::MethodType_Method: bindMethod(method); break;
          }
        }

        if (!m_methods.defaultConstruct.empty())
          m_instance = m_type.call(m_methods.defaultConstruct);
      }

      virtual void update(Level * pLevel, bfc::Timestamp dt) override {}
      virtual void stop(Level * pLevel) override {}
      virtual void pause(Level * pLevel) override {}
      virtual void play(Level * pLevel) override {}
      virtual void deactivate(Level * pLevel) override {}
      virtual void activate(Level * pLevel) override {}
      virtual void collectRenderData(RenderView * pRenderView, Level const * pLevel) override {}

    private:
      void bindConstructor(wren::MethodDesc const& constructDesc) {
        if (constructDesc.numParams == 0 && !m_methods.defaultConstruct.empty())
          m_methods.defaultConstruct = constructDesc.handle();
      }

      void bindMethod(wren::MethodDesc const & methodDesc) {
        if (methodDesc.name == "update")
          m_methods.update = methodDesc.handle();

        if (methodDesc.name == "stop")
          m_methods.stop = methodDesc.handle();

        if (methodDesc.name == "pause")
          m_methods.pause = methodDesc.handle();

        if (methodDesc.name == "play")
          m_methods.play = methodDesc.handle();

        if (methodDesc.name == "deactivate")
          m_methods.deactivate = methodDesc.handle();

        if (methodDesc.name == "activate")
          m_methods.activate = methodDesc.handle();

        if (methodDesc.name == "collectRenderData")
          m_methods.defaultConstruct = methodDesc.handle();
      }

      // Types and instance
      wren::Value m_type;
      wren::Value m_instance;

      struct CallHandles {
        /// Constructors
        wren::Value defaultConstruct;

        // Level events
        wren::Value update;
        wren::Value stop;
        wren::Value pause;
        wren::Value play;
        wren::Value deactivate;
        wren::Value activate;

        // Rendering
        wren::Value collectRenderData;
      } m_methods;
    };

    class EngineAPI {
    public:
      void addComponent(wren::Value const & componentType, const char * uniqueName) {
        
        const char * type = componentType["type"];
        BFC_LOG_INFO("Scripting", "Dynamic component type added: %s", type);
      }

      void addSystem(wren::Value const & systemType, const char * uniqueName) {
        const char * type = systemType["type"];
        BFC_LOG_INFO("Scripting", "Dynamic component type added: %s", type);
      }
    };

    class EngineWrenContext : public WrenContext {
    public:
      EngineWrenContext(Ref<VirtualFileSystem> const & pFS)
        : m_pFS(pFS) {}

      virtual std::optional<URI> resolveModule(URI const & importer, String const & name) override {
        URI importUri = name + ".wren";
        URI cwd       = URI::File(os::getCwd()).resolveRelativeReference(importUri, !importUri.scheme().empty());
        if (m_pFS->exists(cwd)) {
          return cwd;
        }

        if (m_pFS->exists(importUri)) {
          return importUri;
        }

        URI relative = importer.withPath(importer.path().parent()).resolveRelativeReference(importUri, !importUri.scheme().empty());
        if (m_pFS->exists(relative)) {
          return relative;
        }

        return std::nullopt;
      }

      virtual std::optional<String> loadModule(URI const & uri) override {
        String content;
        if (!m_pFS->readText(uri, &content))
          return std::nullopt;
        return content;
      }

    private:
      Ref<VirtualFileSystem> m_pFS = nullptr;
    };

    wren::Module getEngineModule() {
      wren::Module m;

      m.add("EngineAPI") = wren::newForeignType<EngineAPI>()
                               .construct<>()
                               .member(BFC_REFLECT(EngineAPI, addComponent))
                               .member(BFC_REFLECT(EngineAPI, addSystem));

      return m;
    }
  }

  Scripting::Scripting()
    : Subsystem(TypeID<Scripting>(), "Scripting") {}

  bool Scripting::init(Application * pApp) {
    reload();

    return true;
  }

  void Scripting::shutdown() {}

  static void findScripts(URI const & basePath, VirtualFileSystem * pFileSystem, std::function<void(URI const &)> const & cb) {
    for (auto & uri : pFileSystem->walk(basePath)) {
      if (pFileSystem->isLeaf(uri) && uri.path().extension().equals("wren", true)) {
        cb(uri);
      } else {
        findScripts(uri, pFileSystem, cb);
      }
    }
  }

  static void findScripts(VirtualFileSystem * pFileSystem, std::function<void(URI const &)> const & cb) {
    for (auto & drive : pFileSystem->drives()) {
      findScripts(String::format("file:%s:/", drive), pFileSystem, cb);
    }
  }

  void Scripting::reload() {
    auto pFS = getApp()->findSubsystem<VirtualFileSystem>();

    // TODO: Save data
    m_pContext = NewRef<impl::EngineWrenContext>(pFS);
    m_pContext->addForeignModule(URI::File("engine:/scripting/engine.wren"), impl::getEngineModule());
    // m_pContext->addForeignModule();

    findScripts(URI::File("game:/"), pFS.get(), [=](URI const & uri) {
      String code;
      if (pFS->readText(uri, &code)) {
        m_pContext->interpret(uri, code);
      }
    });
  }
} // namespace engine
