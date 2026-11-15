#pragma once

#include "LuaScripting.h"
#include "Behaviours.h"

namespace engine {
  class LuaBehaviourClass;
  class LuaBehaviourInstance : public BehaviourInstance {
  public:
    LuaBehaviourInstance(LuaBehaviourClass * pClass, sol::table instance);

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
    void activate() override;
    /// Call the scripts deactivate method
    void deactivate() override;
    /// Call the scripts update method
    void update(bfc::Timestamp dt) override;
    /// Call the scripts stop method
    void stop() override;
    /// Call the scripts pause method
    void pause() override;
    /// Call the scripts destroy method
    void destroyed() override;

    bool bind(engine::Level * pLevel, engine::EntityID entityID) override;
    bool unbind() override;

    /// Get the class that created this instance
    virtual BehaviourClass * getClass() const override;
    sol::protected_function m_update;
    sol::protected_function m_activate;
    sol::protected_function m_deactivate;
    sol::protected_function m_pause;
    sol::protected_function m_stop;
    sol::protected_function m_destroyed;

    sol::table          m_instance;
    LuaBehaviourClass * m_pClass = nullptr;
  };

  class LuaBehaviourClass : public BehaviourClass {
  public:
    struct Shared {
      sol::protected_function bind;
      sol::protected_function unbind;
      sol::object             nullValue;
    };

    LuaBehaviourClass(sol::state * pLua, sol::table classTable, bfc::Ref<Shared> pContext);

    /// Create an instance of this behaviour class.
    virtual bfc::Ref<BehaviourInstance> instantiate() override;
    /// Serialize an instance of this behaviour class.
    virtual bfc::SerializedObject serialize(BehaviourInstance * pSerialize);
    /// Deserialize an instance of this behaviour class.
    virtual bfc::Ref<BehaviourInstance> deserialize(bfc::SerializedObject const & serialization);
    bool                    bindEntity(sol::table table, engine::Level * pLevel, engine::EntityID entity);
    bool                    unbindEntity(sol::table table);

    sol::table              m_class;
    sol::protected_function m_new;
    bfc::Ref<Shared>        m_pShared = nullptr;
  };
}
