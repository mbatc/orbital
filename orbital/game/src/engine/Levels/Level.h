#pragma once

#include "core/Map.h"
#include "core/Pool.h"
#include "core/Serialize.h"
#include "core/typeindex.h"

namespace engine {
  using EntityID = uint64_t;

  constexpr EntityID InvalidEntity = 0;

  class ILevelComponents {
  public:
    virtual ~ILevelComponents() = default;
  };

  template<typename T>
  class LevelComponents : public ILevelComponents {
  public:
    template<typename... Args>
    T & add(EntityID entityID, Args &&... args) {
      const int64_t index = toIndex(entityID);
      if (index != bfc::npos) {
        BFC_FAIL("Entity %llu already has a %s component", entityID, bfc::TypeID<T>().name());
        return m_components[index];
      }

      m_components.pushBack(T(std::forward<Args>(args)...));

      return m_components.back();
    }

    T & get(EntityID entityID) {
      return m_components[toIndex(entityID)];
    }

    T const & get(EntityID entityID) const {
      return m_components[toIndex(entityID)];
    }

    bool exists(EntityID entityID) const {
      return toIndex(entityID) != bfc::npos;
    }

    bool erase(EntityID entityID) {
      const int64_t index = toIndex(entityID);
      if (index == bfc::npos) {
        return false;
      }

      const int64_t  backComponent = m_components.size() - 1;
      const EntityID backEntity    = m_componentToEntity[backComponent];

      std::swap(m_components[index], m_components.back());

      m_componentToEntity[index]      = backEntity;
      m_entityToComponent[backEntity] = index;

      m_componentToEntity.erase(backComponent);
      m_entityToComponent.erase(entityID);
      m_components.popBack();
    }

    EntityID toEntity(T const * pComponent) const {
      int64_t index = pComponent - m_components.begin();
      if (index < 0 || index >= m_components.size())
        return InvalidEntity;

      return;
    }

    int64_t toIndex(EntityID entityID) const {
      int64_t ret = bfc::npos;
      m_entityToComponent.tryGet(entityID, &ret);
      return ret;
    }

    int64_t size() const {
      return m_components.size();
    }

    int64_t capacity() const {
      return m_components.capacity();
    }

    T * begin() {
      return m_components.begin();
    }

    T * end() {
      return m_components.end();
    }

    T const * begin() const {
      return m_components.begin();
    }

    T const * end() const {
      return m_components.end();
    }

  private:
    bfc::Vector<T>              m_components;
    bfc::Map<EntityID, int64_t> m_entityToComponent;
    bfc::Map<int64_t, EntityID> m_componentToEntity;
  };

  class Level {
  public:
    Level();

    /// Create a new empty entity.
    EntityID create();

    /// Remove an entity from the level.
    bool remove(EntityID const & o);

    /// Number of entities in the level.
    int64_t size() const;

    /// Number of entities allocated in the level.
    int64_t capacity() const;

    /// Test if `entityID` is contained in this scene.
    bool contains(EntityID const & entityID) const;

    /// Add a component to an entity.
    template<typename T, typename... Args>
    T & add(EntityID const & entityID, Args &&... args) {
      return components<T>().add(entityID, std::forward<Args>(args)...);
    }

    /// Remove a component from an entity.
    template<typename T>
    bool erase(EntityID const & entityID) {
      return components<T>().erase(entityID);
    }

    /// Test if an entity has a component.
    template<typename T>
    T & has(EntityID const & entityID) const {
      return components<T>().has(entityID);
    }

    template<typename T>
    LevelComponents<T> & components() {
      bfc::Ref<ILevelComponents> pStorage;
      if (!m_components.tryGet(bfc::TypeID<T>(), &pStorage)) {
        pStorage = bfc::NewRef<LevelComponents<T>>();
        m_components.add(bfc::TypeID<T>(), pStorage);
      }

      return *(LevelComponents<T> const *)pStorage.get();
    }

    template<typename T>
    LevelComponents<T> const & components() const {
      bfc::Ref<ILevelComponents> pStorage;
      if (!m_components.tryGet(bfc::TypeID<T>(), &pStorage)) {
        static const LevelComponents<T> empty;
        return empty;
      }

      return *(LevelComponents<T> const *)pStorage.get();
    }

    /// Unpack the index from an entity ID.
    inline static constexpr int64_t indexOf(EntityID const & entityID) {
      return (entityID & 0x00000000FFFFFFFF);
    }

    /// Unpack the version from an entity ID.
    inline static constexpr int64_t versionOf(EntityID const & entityID) {
      return (entityID & 0xFFFFFFFF00000000) >> 32;
    }

    /// Pack an entity ID from its index and version
    inline static constexpr EntityID toEntityID(uint32_t const & index, uint32_t const & version) {
      return ((uint64_t)index | ((uint64_t)version << 32ll));
    }

  private:
    bool contains(int64_t const & index, int64_t const & version) const;

    int64_t               m_entityCount = 0;
    bfc::Vector<EntityID> m_entities;
    bfc::Vector<EntityID> m_freed;

    bfc::Map<bfc::type_index, bfc::Ref<ILevelComponents>> m_components;
  };
} // namespace engine
