#pragma once

#include "core/Map.h"
#include "core/Pool.h"
#include "core/Serialize.h"
#include "core/typeindex.h"
#include "util/UUID.h"

#include "LevelView.h"

namespace bfc {
  class Events;
}

namespace engine {
  class Level {
  public:
    class EntityView {
    public:
      class Iterator {
      public:
        Iterator(bfc::Span<const EntityID> ids, int64_t index);
        bool       operator==(Iterator const & rhs) const;
        bool       operator!=(Iterator const & rhs) const;
        Iterator & operator++();
        Iterator   operator++(int);
        EntityID   operator*() const;

      private:
        void nextValidIndex();

        bfc::Span<const EntityID> m_ids;
        int64_t                   m_index = 0;
      };

      EntityView(Level const * pLevel)
        : m_pLevel(pLevel) {}

      Iterator begin() const;
      Iterator end() const;

    private:
      Level const * m_pLevel = nullptr;
    };

    Level();
    Level(Level && o);
    Level & operator       =(Level && o);
    Level(Level const & o) = delete;

    ~Level();

    /// Create a new empty entity.
    EntityID create(std::optional<bfc::UUID> const & id = std::nullopt);

    /// Find an entity by using its UUID.
    EntityID find(bfc::UUID const & id) const;

    /// Get the UUID of an entity.
    bfc::UUID uuidOf(EntityID const & o) const;

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
      return addVariant(entityID, 0, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T & replace(EntityID const & entityID, Args &&... args) {
      return replaceVariant(entityID, 0, std::forward<Args>(args)...);
    }

    /// Add a component to an entity.
    template<typename T, typename... Args>
    T & addVariant(EntityID const & entityID, uint32_t variation, Args &&... args) {
      return components<T>(variation).add(entityID, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T & replaceVariant(EntityID const & entityID, uint32_t variation, Args &&... args) {
      return components<T>(variation).replace(entityID, std::forward<Args>(args)...);
    }

    template<typename T>
    T & get(EntityID const & entityID, uint32_t variation = 0) {
      return components<T>(variation).get(entityID);
    }

    template<typename T>
    T const & get(EntityID const & entityID, uint32_t variation = 0) const {
      return components<T>(variation).get(entityID);
    }

    /// Try get a component attached to an entity.
    /// @returns A pointer to the component attached
    /// @retval nullptr If the entity does not have a component of type `T` attached.
    template<typename T>
    T * tryGet(EntityID const & entityID, uint32_t variation = 0) {
      return components<T>(variation).tryGet(entityID);
    }

    template<typename T>
    T const * tryGet(EntityID const & entityID, uint32_t variation = 0) const {
      return components<T>(variation).tryGet(entityID);
    }

    /// Remove a component from an entity.
    template<typename T>
    bool erase(EntityID const & entityID, uint32_t variation = 0) {
      return components<T>(variation).erase(entityID);
    }

    /// Test if an entity has a component.
    template<typename T>
    bool has(EntityID const & entityID, uint32_t variation = 0) const {
      return components<T>(variation).exists(entityID);
    }

    template<typename T>
    EntityID toEntity(T const * pComponent, uint32_t variation = 0) const {
      return components<T>(variation).toEntity(pComponent);
    }

    EntityView entities() const;

    bfc::Map<ComponentTypeID, bfc::Ref<ILevelComponentStorage>> const & components() const;

    template<typename T>
    LevelComponentStorage<T> & components(uint32_t variation = 0) {
      bfc::Ref<ILevelComponentStorage> pStorage;
      if (!m_components.tryGet(ComponentTypeID(bfc::TypeID<T>(), variation), &pStorage)) {
        pStorage = bfc::NewRef<LevelComponentStorage<T>>(this);
        m_components.add(bfc::TypeID<T>(), pStorage);
      }

      return *(LevelComponentStorage<T> *)pStorage.get();
    }

    template<typename T>
    LevelComponentStorage<T> const & components(uint32_t variation = 0) const {
      bfc::Ref<ILevelComponentStorage> pStorage;
      if (!m_components.tryGet(ComponentTypeID(bfc::TypeID<T>(), variation), &pStorage)) {
        static const LevelComponentStorage<T> empty(nullptr);
        return empty;
      }

      return *(LevelComponentStorage<T> const *)pStorage.get();
    }

    template<typename... Components>
    LevelView<Components...> getView() const {
      return {&components<Components>()...};
    }

    template<typename... Components>
    LevelViewMutable<Components...> getView() {
      return {&components<Components>()...};
    }

    /// Copy entities to pDstLevel
    bfc::Vector<EntityID> copyTo(Level * pDstLevel, bfc::Span<EntityID const> const & entities, bool preserveUUIDs = false);

    /// Copy an entity to pDstLevel
    bfc::Vector<EntityID> copyTo(Level * pDstLevel, bool preserveUUIDs = false);

    /// Copy an entity to pDstLevel
    bfc::Vector<EntityID> copyTo(Level * pDstLevel, EntityID const & entity, bool preserveUUIDs = false);

    /// Copy an entity in this scene.
    EntityID copy(EntityID const & entity);

    /// Remove all entities from the scene.
    void clear();

    /// Add some data to the level.
    /// Data is an arbitrary structure
    template<typename T, typename... Args>
    bfc::Ref<T> addData(Args &&... args) {
      const bfc::type_index type = bfc::TypeID<T>();
      if (m_data.contains(type)) {
        return nullptr;
      }
      auto pNewRef = bfc::NewRef<T>(std::forward<Args>(args)...);
      m_data.add(type, pNewRef);
      return pNewRef;
    }

    /// Get level data.
    template<typename T>
    bfc::Ref<const T> getData() const {
      const bfc::type_index type = bfc::TypeID<T>();
      bfc::Ref<void>        val;
      m_data.tryGet(type, val);
      return std::static_pointer_cast<const T>(val);
    }

    template<typename T>
    bfc::Ref<T> getData() {
      const bfc::type_index type = bfc::TypeID<T>();
      bfc::Ref<void>        val;
      m_data.tryGet(type, &val);
      return std::static_pointer_cast<T>(val);
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

    std::optional<bfc::URI> sourceUri; ///< Where was the level loaded from

  private:
    bool contains(int64_t const & index, int64_t const & version) const;

    bfc::Ref<bfc::Events>                     m_pEvents;
    bfc::Map<bfc::type_index, bfc::Ref<void>> m_data;

    int64_t               m_entityCount = 0;
    bfc::Vector<EntityID> m_entities;
    bfc::Vector<EntityID> m_freed;

    bfc::Vector<bfc::UUID>        m_ids;
    bfc::Map<bfc::UUID, EntityID> m_idToEntity;

    bfc::Map<ComponentTypeID, bfc::Ref<ILevelComponentStorage>> m_components;
  };
} // namespace engine
