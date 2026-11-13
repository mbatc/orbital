#include "Behaviours.h"
#include "Levels/Level.h"
#include "Application.h"

namespace engine {
  namespace impl {
    bfc::Map<bfc::String, bfc::Ref<engine::BehaviourClass>> behaviours;
    bfc::Map<engine::BehaviourClass const *, bfc::String> behaviourToName;
  }

  bool BehaviourClass::Registry::add(bfc::StringView const & identifier, bfc::Ref<BehaviourClass> pClass) {
    return impl::behaviours.tryAdd(identifier, pClass);
  }

  bool BehaviourClass::Registry::remove(bfc::StringView const & identifier) {
    return impl::behaviours.erase(identifier);
  }

  bfc::Ref<BehaviourClass> BehaviourClass::Registry::find(bfc::StringView const & identifier) {
    return impl::behaviours.getOr(identifier, nullptr);
  }

  bfc::Vector<bfc::String> BehaviourClass::Registry::identifiers() {
    return impl::behaviours.getKeys();
  }

  bfc::String BehaviourClass::Registry::identifierOf(BehaviourClass const * pClass) {
    return impl::behaviourToName.getOr(pClass, "");
  }

  bfc::String BehaviourClass::Registry::identifierOf(BehaviourInstance const * pInstance) {
    return identifierOf(pInstance->getClass());
  }
} // namespace engine

namespace components {
  bool Behaviours::has(engine::BehaviourClass const * pClass) const {
    return m_instances.find([pClass](auto pInstance) { return pInstance->getClass() == pClass; }) != -1;
  }

  bool Behaviours::add(engine::Level * pLevel, engine::EntityID entity, engine::BehaviourClass * pBehaviour) {
    if (has(pBehaviour))
      return false;
    auto pInstance = pBehaviour->instantiate();
    if (!pInstance->bind(pLevel, entity))
      return false;
    m_instances.pushBack(pInstance);
    return true;
  }

  bool Behaviours::add(engine::Level * pLevel, engine::EntityID entity, bfc::Ref<engine::BehaviourInstance> pInstance) {
    if (has(pInstance->getClass()) || !pInstance->bind(pLevel, entity))
      return false;
    m_instances.pushBack(pInstance);
    return true;
  }

  bool Behaviours::remove(engine::BehaviourClass * pClass) {
    const int64_t idx = m_instances.find([pClass](auto pInstance) { return pInstance->getClass() == pClass; });
    return m_instances.erase(idx);
  }

  bfc::Vector<bfc::Ref<engine::BehaviourInstance>> const & Behaviours::instances() const {
    return m_instances;
  }
}

namespace engine {
  void BehavioursLevelSystem::update(Level * pLevel, bfc::Timestamp dt) {
    for (auto & [script] : pLevel->getView<components::Behaviours>()) {
      for (auto & instance : script.instances())
        instance->update(pLevel, dt);
    }
  }

  void BehavioursLevelSystem::activate(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>()) {
      for (auto & instance : script.instances())
        instance->activate(pLevel);
    }
  }

  void BehavioursLevelSystem::deactivate(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>())
      for (auto & instance : script.instances())
        instance->deactivate(pLevel);
  }

  void BehavioursLevelSystem::pause(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>())
      for (auto & instance : script.instances())
        instance->pause(pLevel);
  }

  void BehavioursLevelSystem::stop(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>())
      for (auto & instance : script.instances())
        instance->stop(pLevel);
  }

  void BehavioursLevelSystem::destroyed(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>())
      for (auto & instance : script.instances())
        instance->destroyed(pLevel);
  }

  BehavioursPlugin::BehavioursPlugin()
    : Subsystem(bfc::TypeID<BehavioursPlugin>(), "Behaviours") {}

  bool BehavioursPlugin::init(Application*) {
    registerLevelSystem<BehavioursLevelSystem>();
    registerComponentType<components::Behaviours>("Behaviours");
    return true;
  }

  void BehavioursPlugin::shutdown() {}

  void BehavioursPlugin::loop(Application* ) {}
} // namespace engine
