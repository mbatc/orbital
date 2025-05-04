#include "Scripting.h"
#include "Application.h"
#include "Input.h"

#include "util/Log.h"

#include "Assets/VirtualFileSystem.h"
#include "Assets/AssetManager.h"
#include "Assets/AssetLoader.h"

#include "scripting/WrenContext.h"
#include "platform/OS.h"

#include "Levels/Level.h"
#include "Levels/LevelSystem.h"
#include "Levels/LevelComponents.h"

using namespace bfc;
using namespace scripting;
using namespace components;

namespace engine {
  template<>
  struct LevelComponent_OnPreErase<Script> {
    inline static void onPreErase(Script * pComponent, Level * pLevel) {

    }
  };

  template<>
  struct LevelComponent_OnPostAdd<Script> {
    inline static void onPostAdd(Script * pComponent, Level * pLevel) {

    }
  };

  namespace impl {
    class WrenTypeSerializer {
    public:


      bfc::Vector<bfc::Pair<bfc::String, wren::Value>> m_fields;
    };

    class ScriptComponentTypeAdapter : public ILevelComponentType {
    public:
      ScriptComponentTypeAdapter(wren::Value type, uint32_t const & variation)
        : m_scriptType(type)
        , m_variation(variation) {}

      virtual ComponentTypeID type() const {
        return ComponentTypeID(bfc::TypeID<Script>(), m_variation);
      }

      virtual SerializedObject write(EntityID entity, ComponentSerializeContext const & context) const {
        BFC_UNUSED(entity, context);

        // if (auto pScript = context.pLevel->tryGet<Script>(entity)) {
        //   Set<String> seralizeable;
        //   for (auto & method : pScript->instance.methods()) {
        //     if (method.type == wren::MethodType_Setter) {
        //       seralizeable.add(method.name);
        //     }
        //   }
        // 
        //   Map<String, SerializedObject> ret;
        //   for (auto & method : pScript->instance.methods()) {
        //     if (method.type == wren::MethodType_Getter && seralizeable.contains(method.name)) {
        //       ret.add(method.name, );
        // 
        //     }
        //   }
        // 
        // 
        //   return SerializedObject::MakeMap(std::move(ret));
        // }
        // 
        return SerializedObject::Empty();
      }

      virtual bool read(SerializedObject const & serialized, EntityID entity, ComponentDeserializeContext const & context) const {
        BFC_UNUSED(serialized, entity, context);
        return false;
      }

      virtual bool copy(LevelCopyContext * pContext, Level* pDstLevel, EntityID dstEntity, Level const& srcLevel, EntityID srcEntity) const {
        if (auto * pScript = srcLevel.tryGet<Script>(srcEntity, m_variation)) {
          auto & newScript   = pDstLevel->addVariant<Script>(dstEntity, m_variation);
          newScript.type     = pScript->type;
          newScript.instance = pScript->type.call(m_methods.construct);
          return true;
        }

        return false;
      }

      virtual void * addComponent(Level * pDstLevel, EntityID entity) const {
        auto & newScript   = pDstLevel->addVariant<Script>(entity, m_variation);
        newScript.type     = m_scriptType;
        newScript.instance = newScript.type.call(m_methods.construct);

        // TODO: Cache wren::Value in Level::getData()
        wren::Value(pDstLevel).call("add", m_scriptType, entity);

        return &newScript;
      }

      bool bind() {
        m_componentName = m_scriptType["type"];

        for (auto& method : m_scriptType.methods()) {
          switch (method.type) {
            case m_scriptType["type"];
          }
        }
      }

      void unbind() {

      }

      bfc::String getComponentName() const {
        return m_componentName;
      }

    private:
      wren::Value m_scriptType;
      String      m_componentName;
      uint32_t    m_variation = 0;

      struct Methods {
        wren::Value construct;
      } m_methods;
    };

    class LevelSystemAdapter
      : public engine::ILevelUpdate
      , public engine::ILevelStop
      , public engine::ILevelPause
      , public engine::ILevelPlay
      , public engine::ILevelDeactivate
      , public engine::ILevelActivate
      , public engine::ILevelRenderDataCollector {
    public:
      LevelSystemAdapter(wren::Value const & systemType)
        : m_type(systemType) {}

      bool bind() {
        for (auto & method : m_type.methods()) {
          switch (method.type) {
          case wren::MethodType_Constructor: bindConstructor(method); break;
          case wren::MethodType_Method: bindMethod(method); break;
          }
        }

        if (!m_methods.defaultConstruct.empty())
          m_instance = m_type.call(m_methods.defaultConstruct);

        return !m_instance.empty();
      }

      bool hasUpdate() const {
        return !m_methods.update.empty();
      }
      virtual void update(Level * pLevel, bfc::Timestamp dt) override {
        m_instance.call(m_methods.update, pLevel, dt.secs());
      }

      bool hasStop() const {
        return !m_methods.stop.empty();
      }
      virtual void stop(Level * pLevel) override {
        m_instance.call(m_methods.stop, pLevel);
      }

      bool hasPause() const {
        return !m_methods.pause.empty();
      }
      virtual void pause(Level * pLevel) override {
        m_instance.call(m_methods.pause, pLevel);
      }

      bool hasPlay() const {
        return !m_methods.play.empty();
      }
      virtual void play(Level * pLevel) override {
        m_instance.call(m_methods.play, pLevel);
      }

      bool hasDeactivate() const {
        return !m_methods.deactivate.empty();
      }
      virtual void deactivate(Level * pLevel) override {
        m_instance.call(m_methods.deactivate, pLevel);
      }

      bool hasActivate() const {
        return !m_methods.activate.empty();
      }
      virtual void activate(Level * pLevel) override {
        m_instance.call(m_methods.activate, pLevel);
      }

      bool hasCollectRenderData() const {
        return !m_methods.collectRenderData.empty();
      }
      virtual void collectRenderData(RenderView * pRenderView, Level const * pLevel) override {
        m_instance.call(m_methods.collectRenderData, pRenderView, pLevel);
      }

    private:
      void bindConstructor(wren::MethodDesc const& constructDesc) {
        if (constructDesc.numParams == 0 && !m_methods.defaultConstruct.empty())
          m_methods.defaultConstruct = constructDesc.handle();
      }

      void bindMethod(wren::MethodDesc const & methodDesc) {
        if (methodDesc.name == "update" && methodDesc.numParams == 2)
          m_methods.update = methodDesc.handle();

        if (methodDesc.name == "stop" && methodDesc.numParams == 1)
          m_methods.stop = methodDesc.handle();

        if (methodDesc.name == "pause" && methodDesc.numParams == 1)
          m_methods.pause = methodDesc.handle();

        if (methodDesc.name == "play" && methodDesc.numParams == 1)
          m_methods.play = methodDesc.handle();

        if (methodDesc.name == "deactivate" && methodDesc.numParams == 1)
          m_methods.deactivate = methodDesc.handle();

        if (methodDesc.name == "activate" && methodDesc.numParams == 1)
          m_methods.activate = methodDesc.handle();

        if (methodDesc.name == "collectRenderData" && methodDesc.numParams == 2)
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
      void addComponent(wren::Value const & componentType) {
        m_unboundComponentTypes.pushBack(bfc::NewRef<ScriptComponentTypeAdapter>(componentType, m_variation++));
      }

      void addSystem(wren::Value const & systemType) {
        m_unboundSystems.pushBack(bfc::NewRef<LevelSystemAdapter>(systemType));
      }

      void bind() {
        for (auto& scriptType : m_unboundComponentTypes) {
          if (scriptType->bind()) {
            m_componentTypes.pushBack(scriptType);

            registerComponentType(scriptType->getComponentName(), scriptType);

            BFC_LOG_INFO("Scripting", "Dynamic component type added: %s", scriptType->getComponentName());
          }
        }

        for (auto & system : m_unboundSystems) {
          if (!system->bind()) {
            continue;
          }

          if (system->hasActivate())
            registerLevelActivate(system);

          if (system->hasDeactivate())
            registerLevelDeactivate(system);

          if (system->hasPlay())
            registerLevelPlay(system);

          if (system->hasUpdate())
            registerLevelUpdate(system);

          if (system->hasPause())
            registerLevelPause(system);

          if (system->hasStop())
            registerLevelStop(system);

          if (system->hasCollectRenderData())
            registerLevelRenderDataCollector(system);

          m_systems.pushBack(system);
        }

        m_unboundSystems.clear();
        m_unboundComponentTypes.clear();
      }

    private:
      uint32_t                                          m_variation = 0;
      bfc::Vector<bfc::Ref<ScriptComponentTypeAdapter>> m_unboundComponentTypes;
      bfc::Vector<bfc::Ref<LevelSystemAdapter>>         m_unboundSystems;

      bfc::Vector<bfc::Ref<ScriptComponentTypeAdapter>> m_componentTypes;
      bfc::Vector<bfc::Ref<LevelSystemAdapter>>         m_systems;
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
