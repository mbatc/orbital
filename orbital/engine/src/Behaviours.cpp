#include "Behaviours.h"
#include "Levels/Level.h"
#include "Application.h"

namespace engine {
  namespace impl {
    bfc::Map<bfc::String, bfc::Ref<BehaviourClass>>       behaviours;
    bfc::Map<engine::BehaviourClass const *, bfc::String> behaviourToName;
  }

  bool BehaviourClass::Registry::add(bfc::StringView const & identifier, bfc::Ref<BehaviourClass> pClass) {
    if (impl::behaviours.tryAdd(identifier, pClass)) {
      if (impl::behaviourToName.tryAdd(pClass.get(), identifier)) {
        return true;
      } else
        impl::behaviours.erase(identifier);
    }

    return false;
  }

  bool BehaviourClass::Registry::remove(bfc::StringView const & identifier) {
    bfc::Ref<engine::BehaviourClass> pClass;
    if (!impl::behaviours.erase(identifier, &pClass))
      return false;
    impl::behaviourToName.erase(pClass.get());
    return true;
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
    const int64_t idx = m_instances.find([pClass](auto pInstance) {
      return pInstance->getClass() == pClass;
    });

    m_instances[idx]->deactivate();
    m_instances[idx]->unbind();

    return m_instances.erase(idx);
  }

  bfc::Vector<bfc::Ref<engine::BehaviourInstance>> const & Behaviours::instances() const {
    return m_instances;
  }

  bool Behaviours::ComponentTraits::copy(engine::LevelCopyContext * pContext, engine::Level * pDstLevel,
    engine::EntityID dstEntity, engine::Level const& srcLevel,
    Behaviours const& component) {
    Behaviours * pDstBehaviours = pDstLevel->tryGet<Behaviours>(dstEntity);
    if (pDstBehaviours == nullptr) {
      pDstBehaviours = &pDstLevel->add<Behaviours>(dstEntity);
    }

    for (const bfc::Ref<engine::BehaviourInstance> &pInstance : component.instances()) {
      const std::optional<bfc::SerializedObject> serialized = pInstance->serialize();
      if (!serialized.has_value())
        return false;

      const bfc::Ref<engine::BehaviourInstance> pNewInstance = pInstance->getClass()->deserialize(serialized.value());
      if (pNewInstance == nullptr)
        return false;

      pDstBehaviours->add(pDstLevel, dstEntity, pNewInstance);
    }

    return true;
  }

  bool Behaviours::ComponentTraits::preErase(Behaviours * pComponent, engine::Level * pLevel) {
    pLevel;
    for (const bfc::Ref<engine::BehaviourInstance> pInstance : pComponent->instances()) {
      pInstance->deactivate();
      pInstance->destroyed();
      pInstance->unbind();
    }
    return true;
  }

  bfc::SerializedObject Behaviours::Serialization::write(components::Behaviours const &            o,
                                                           engine::ComponentSerializeContext const & ctx) {
    bfc::Map<bfc::String, bfc::SerializedObject> instances;
    for (auto & pInstance : o.instances()) {
      instances.add(engine::BehaviourClass::Registry::identifierOf(pInstance.get()), pInstance->serialize());
    }

    return bfc::SerializedObject::MakeMap({
      {"instances", bfc::SerializedObject::MakeMap(instances)},
    });
  }

  bool Behaviours::Serialization::read(bfc::SerializedObject const & s, components::Behaviours & o,
                                              engine::ComponentDeserializeContext const & ctx) {
    bfc::mem::construct(&o);

    bfc::SerializedObject const & instances = s.get("instances");
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
} // namespace components

namespace engine {
  void BehavioursLevelSystem::update(Level * pLevel, bfc::Timestamp dt) {
    for (auto & [script] : pLevel->getView<components::Behaviours>()) {
      for (auto & instance : script.instances())
        instance->update(dt);
    }
  }

  void BehavioursLevelSystem::activate(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>()) {
      for (auto & instance : script.instances())
        instance->activate();
    }
  }

  void BehavioursLevelSystem::deactivate(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>())
      for (auto & instance : script.instances())
        instance->deactivate();
  }

  void BehavioursLevelSystem::pause(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>())
      for (auto & instance : script.instances())
        instance->pause();
  }

  void BehavioursLevelSystem::stop(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>())
      for (auto & instance : script.instances())
        instance->stop();
  }

  void BehavioursLevelSystem::destroyed(Level * pLevel) {
    for (auto & [script] : pLevel->getView<components::Behaviours>()) {
      for (auto & instance : script.instances()) {
        instance->destroyed();
        instance->unbind();
      }
    }
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
