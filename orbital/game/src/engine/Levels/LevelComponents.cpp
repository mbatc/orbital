#include "LevelComponents.h"

namespace engine {
  static bfc::Pool<bfc::Ref<ILevelComponentType>> g_interfaces;
  static bfc::Map<bfc::String, int64_t>           g_nameLookup;
  static bfc::Map<bfc::type_index, int64_t>       g_typeLookup;

  bool ILevelComponentType::add(bfc::StringView const & name, bfc::Ref<ILevelComponentType> const & pType) {
    if (g_nameLookup.contains(name)) {
      return false;
    }
    if (g_typeLookup.contains(pType->type())) {
      return false;
    }

    int64_t index = g_interfaces.emplace(pType);
    g_nameLookup.add(name, index);
    g_typeLookup.add(pType->type(), index);
    return true;
  }

  bfc::Ref<ILevelComponentType> ILevelComponentType::find(bfc::StringView const & name) {
    int64_t index = bfc::npos;
    if (!g_nameLookup.tryGet(name, &index)) {
      return nullptr;
    }
    return g_interfaces[index];
  }

  bfc::Ref<ILevelComponentType> ILevelComponentType::find(bfc::type_index const & type) {
    int64_t index = bfc::npos;
    if (!g_typeLookup.tryGet(type, &index)) {
      return nullptr;
    }
    return g_interfaces[index];
  }

  bfc::StringView ILevelComponentType::findName(bfc::type_index const & type) {
    auto pInterface = find(type);
    if (pInterface == nullptr) {
      return "";
    }

    for (auto& [name, index] : g_nameLookup) {
      if (pInterface == g_interfaces[index]) {
        return name;
      }
    }

    return "";
  }

  bfc::Vector<bfc::String> ILevelComponentType::names() {
    return g_nameLookup.getKeys();
  }

  bfc::Vector<bfc::type_index> ILevelComponentType::types() {
    return g_typeLookup.getKeys();
  }

  Level * ILevelComponentStorage::getOwner() const {
    return m_pLevel;
  }

  ILevelComponentStorage::ILevelComponentStorage(Level * pOwner)
    : m_pLevel(pOwner) {}

  namespace impl {
    void ComponentStorageLevelAccess::SetOwner(ILevelComponentStorage * pStorage, Level * pLevel) {
      pStorage->m_pLevel = pLevel;
    }
  } // namespace impl
} // namespace engine
