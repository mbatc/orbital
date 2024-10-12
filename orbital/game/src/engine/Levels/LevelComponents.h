#pragma once

#include "core/Map.h"
#include "core/Pool.h"
#include "core/Serialize.h"
#include "core/typeindex.h"

namespace engine {
  using EntityID = uint64_t;

  constexpr EntityID InvalidEntity = 0;

  class Level;
  class LevelSerializer;
  class ILevelComponentType {
  public:
    /// Add a new component type interface.
    /// @param name  The name of the component type. This must be unique.
    /// @param pType An instance of the type interface.
    static bool add(bfc::StringView const & name, bfc::Ref<ILevelComponentType> const & pType);

    /// Find the component type interface for a specific type by name.
    static bfc::Ref<ILevelComponentType> find(bfc::StringView const & name);

    /// Find the component type interface for a specific type.
    static bfc::Ref<ILevelComponentType> find(bfc::type_index const & type);

    /// Find the name of a component by type.
    static bfc::StringView findName(bfc::type_index const & type);

    /// Get all registered component names.
    static bfc::Vector<bfc::String> names();

    /// Get all registered component types.
    static bfc::Vector<bfc::type_index> types();

    virtual bfc::type_index type() const = 0;

    virtual bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, EntityID entity) const = 0;

    virtual bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & serialized, Level & level, EntityID entity) const = 0;

    virtual bool copy(Level * pDstLevel, EntityID dstEntity, Level const * pSrcLevel, EntityID srcEntity) const = 0;
  };

  template<typename T>
  class LevelComponentType : public ILevelComponentType {
    virtual bfc::type_index type() const override {
      return bfc::TypeID<T>();
    }

    virtual bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, EntityID entity) const override {
      BFC_UNUSED(pSerializer);

      T const & component = level.get<T>(entity);

      return bfc::serialize(component);
    }

    virtual bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & serialized, Level & level, EntityID entity) const override {
      BFC_UNUSED(pSerializer);

      bfc::Uninitialized<T> component;
      if (!serialized.read(component.get())) {
        return false;
      }

      level.replace<T>(entity, component.take());
      return true;
    }

    virtual bool copy(Level * pDstLevel, EntityID dstEntity, Level const * pSrcLevel, EntityID srcEntity) const override {
      if (T const* pSrcComponent = pSrcLevel->tryGet<T>(srcEntity)) {
        pDstLevel->replace<T>(dstEntity, *pSrcComponent);
        return true;
      }

      return false;
    }
  };

  template<typename T>
  bool registerComponentType(bfc::StringView const & name) {
    return ILevelComponentType::add(name, bfc::NewRef<LevelComponentType<T>>());
  }

  class ILevelComponentStorage {
  public:
    virtual ~ILevelComponentStorage() = default;

    virtual bfc::type_index type() const = 0;

    virtual bool exists(EntityID entityID) const = 0;
    virtual bool erase(EntityID entityID)        = 0;

    virtual int64_t size() const     = 0;
    virtual int64_t capacity() const = 0;

    virtual EntityID toEntity(void const * pComponent) const = 0;
    virtual int64_t  toIndex(EntityID entityID) const        = 0;

    virtual bfc::Span<const EntityID> entities() const = 0;
  };

  template<typename T>
  class LevelComponentStorage : public ILevelComponentStorage {
  public:
    virtual bfc::type_index type() const override {
      return bfc::TypeID<T>();
    }

    virtual bool exists(EntityID entityID) const override {
      return toIndex(entityID) != bfc::npos;
    }

    virtual bool erase(EntityID entityID) override {
      const int64_t index = toIndex(entityID);
      if (index == bfc::npos) {
        return false;
      }

      const int64_t  backComponent = m_components.size() - 1;
      const EntityID backEntity    = m_componentToEntity[backComponent];

      std::swap(m_components[index], m_components.back());
      std::swap(m_componentToEntity[index], m_componentToEntity.back());
      m_entityToComponent[backEntity] = index;

      m_entityToComponent.erase(entityID);
      m_componentToEntity.popBack();
      m_components.popBack();
      return true;
    }

    virtual EntityID toEntity(void const * pComponent) const override {
      int64_t index = (T const *)pComponent - m_components.begin();
      if (index < 0 || index >= m_components.size())
        return InvalidEntity;

      return m_componentToEntity[index];
    }

    virtual int64_t toIndex(EntityID entityID) const override {
      int64_t ret = bfc::npos;
      m_entityToComponent.tryGet(entityID, &ret);
      return ret;
    }

    virtual int64_t size() const override {
      return m_components.size();
    }

    virtual int64_t capacity() const override {
      return m_components.capacity();
    }

    template<typename... Args>
    T & add(EntityID entityID, Args &&... args) {
      const int64_t index = toIndex(entityID);
      if (index != bfc::npos) {
        BFC_FAIL("Entity %llu already has a %s component", entityID, bfc::TypeID<T>().name());
        return m_components[index];
      }

      m_entityToComponent.add(entityID, m_components.size());
      m_componentToEntity.pushBack(entityID);
      m_components.pushBack(T(std::forward<Args>(args)...));
      return m_components.back();
    }

    template<typename... Args>
    T & replace(EntityID entityID, Args &&... args) {
      const int64_t index = toIndex(entityID);

      if (index != bfc::npos) {
        mem::destruct(m_components.begin() + index);
        mem::construct(m_components.begin() + index, std::forward<Args>(args)...);
        return m_components[index];
      }

      return add(entityID, std::forward<Args>(args)...);
    }

    T & get(EntityID entityID) {
      return m_components[toIndex(entityID)];
    }

    T const & get(EntityID entityID) const {
      return m_components[toIndex(entityID)];
    }

    T * tryGet(EntityID entityID) {
      const int64_t index = toIndex(entityID);
      if (index == bfc::npos) {
        return nullptr;
      }
      return &m_components[toIndex(entityID)];
    }

    T const * tryGet(EntityID entityID) const {
      const int64_t index = toIndex(entityID);
      if (index == bfc::npos) {
        return nullptr;
      }
      return &m_components[toIndex(entityID)];
    }

    bfc::Span<T> components() {
      return m_components;
    }

    bfc::Span<const T> components() const {
      return m_components;
    }

    virtual bfc::Span<const EntityID> entities() const override {
      return m_componentToEntity;
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
    bfc::Vector<EntityID>       m_componentToEntity;
    bfc::Map<EntityID, int64_t> m_entityToComponent;
  };
} // namespace engine
