#pragma once

#include "Subsystem.h"
#include "core/Timestamp.h"
#include "core/Serialize.h"
#include "Levels/LevelSystem.h"
#include "Levels/LevelComponents.h"
#include <optional>

namespace engine {
  class Level;
  class BehaviourClass;
  class BehaviourInstance {
  public:
    /// Call the behaviours activate method
    virtual void activate() = 0;

    /// Call the behaviours deactivate method
    virtual void deactivate() = 0;

    /// Call the behaviours update method
    virtual void update(bfc::Timestamp dt) = 0;

    /// Call the behaviours stop method
    virtual void stop() = 0;

    /// Call the behaviours pause method
    virtual void pause() = 0;

    /// Call the behaviours destroy method
    virtual void destroyed() = 0;

    /// Bind this instance to an entity
    virtual bool bind(engine::Level * pLevel, engine::EntityID entity) = 0;

    /// Unbind this instance from the attached entity
    virtual bool unbind() = 0;

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

    struct ComponentTraits {
      static bool copy(engine::LevelCopyContext * pContext, engine::Level * pDstLevel, engine::EntityID dstEntity,
                       engine::Level const & srcLevel, Behaviours const & component);
      static bool preErase(Behaviours * pComponent, engine::Level * pLevel);
    };

    struct Serialization {
      static bfc::SerializedObject write(components::Behaviours const & o, engine::ComponentSerializeContext const & ctx);

      static bool read(bfc::SerializedObject const & s, components::Behaviours & o,
                       engine::ComponentDeserializeContext const & ctx);
    };
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
