#pragma once

#include "Subsystem.h"
#include "core/Timestamp.h"
#include "core/Serialize.h"
#include "Levels/LevelSystem.h"
#include "Levels/LevelSerializer.h"
#include <optional>

namespace engine {
  class Level;
  class BehaviourClass;
  class BehaviourInstance {
  public:
    /// Call the behaviours activate method
    virtual void activate(Level * pLevel) = 0;

    /// Call the behaviours deactivate method
    virtual void deactivate(Level * pLevel) = 0;

    /// Call the behaviours update method
    virtual void update(Level * pLevel, bfc::Timestamp dt) = 0;

    /// Call the behaviours stop method
    virtual void stop(Level * pLevel) = 0;

    /// Call the behaviours pause method
    virtual void pause(Level * pLevel) = 0;

    /// Call the behaviours destroy method
    virtual void destroyed(Level * pLevel) = 0;

    /// Bind this instance to an entity
    virtual bool bind(engine::Level * pLevel, engine::EntityID entity) = 0;

    /// Get the class that created this instance
    virtual BehaviourClass* getClass() const = 0;

    inline bfc::SerializedObject serialize();
  };

  class BehaviourClass {
  public:
    struct Registry {
      static bool                     add(bfc::StringView const & identifier, bfc::Ref<BehaviourClass> pClass);
      static bool                     remove(bfc::StringView const & identifier);
      static bfc::Ref<BehaviourClass> find(bfc::StringView const & identifier);
      static bfc::Vector<bfc::String> identifiers();
      static bfc::String              identifierOf(BehaviourClass const * pClass);
      static bfc::String              identifierOf(BehaviourInstance const * pInstance);
    };

    /// Create an instance of this behaviour class.
    virtual bfc::Ref<BehaviourInstance> instantiate() = 0;

    /// Serialize an instance of this behaviour class.
    virtual bfc::SerializedObject serialize(BehaviourInstance *pSerialize) = 0;

    /// Deserialize an instance of this behaviour class.
    virtual bfc::Ref<BehaviourInstance> deserialize(bfc::SerializedObject const & serialization) = 0;
  };

  inline bfc::SerializedObject BehaviourInstance::serialize() {
    return getClass()->serialize(this);
  }
}

namespace components {
  class Behaviours {
  public:
    bool has(engine::BehaviourClass const * pClass) const;
    bool add(engine::Level * pLevel, engine::EntityID entity, engine::BehaviourClass * pBehaviour);
    bool add(engine::Level * pLevel, engine::EntityID entity, bfc::Ref<engine::BehaviourInstance> pInstance);
    bool remove(engine::BehaviourClass * pClass);
    bfc::Vector<bfc::Ref<engine::BehaviourInstance>> const & instances() const;

  private:
    bfc::Vector<bfc::Ref<engine::BehaviourInstance>> m_instances;
  };
}

namespace engine {
  class BehavioursLevelSystem
    : public ILevelDestroyed
    , public ILevelUpdate
    , public ILevelActivate
    , public ILevelDeactivate
    , public ILevelPause
    , public ILevelStop {
  public:
    void destroyed(Level * pLevel) override;
    void update(Level * pLevel, bfc::Timestamp dt) override;
    void activate(Level * pLevel) override;
    void deactivate(Level * pLevel) override;
    void pause(Level * pLevel) override;
    void stop(Level * pLevel) override;
  };

  class BehavioursPlugin : public Subsystem {
  public:
    BehavioursPlugin();

    virtual bool init(Application * pApp) override;
    virtual void shutdown() override;
    virtual void loop(Application * pApp) override;
  };
}

namespace bfc {
  template<>
  struct Serializer<components::Behaviours> {
    static SerializedObject write(components::Behaviours const & o, engine::ComponentSerializeContext const & ctx) {
      Map<String, SerializedObject> instances;
      for (auto& pInstance : o.instances()) {
        instances.add(engine::BehaviourClass::Registry::identifierOf(pInstance.get()), pInstance->serialize());
      }

      return SerializedObject::MakeMap({
        { "instances", SerializedObject::MakeMap(instances) },
      });
    }

    static bool read(SerializedObject const & s, components::Behaviours & o,
                     engine::ComponentDeserializeContext const & ctx) {
      mem::construct(&o);

      SerializedObject const & instances = s.get("instances");
      if (!instances.isMap()) {
        return false;
      }

      for (auto & [name, serialization] : instances.asMap()) {
        auto pBehaviourClass = engine::BehaviourClass::Registry::find(name);
        if (pBehaviourClass == nullptr)
          continue;

        auto pInstance = pBehaviourClass->deserialize(serialization);
        if (pInstance != nullptr) {
          o.add(ctx.pLevel, ctx.entity, pInstance);
        }
      }

      return true;
    }
  };
}
