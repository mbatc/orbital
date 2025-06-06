#pragma once

#include "core/Map.h"
#include "core/Pool.h"
#include "core/Serialize.h"
#include "core/typeindex.h"
#include "LevelSerializer.h"

namespace engine {
  using EntityID = uint64_t;

  constexpr EntityID InvalidEntity = 0;

  class Level;
  class LevelSerializer;
  class LevelCopyContext {
  public:
    LevelCopyContext(Level * pDst, Level const * pSrc);
    ~LevelCopyContext();

    EntityID remap(EntityID const & src) const;
    bool     remap(EntityID * pEntityID) const;

    bool addMappedEntity(EntityID const & dst, EntityID const & src);
    void defer(std::function<void(LevelCopyContext *, Level *)> const & func);

  private:
    Level *       m_pDstLevel = nullptr;
    Level const * m_pSrcLevel = nullptr;

    bfc::Vector<std::function<void(LevelCopyContext *, Level *)>> m_deferred;
    bfc::Map<EntityID, EntityID>                                  m_mappedEntities;
  };

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

    virtual bfc::SerializedObject write(EntityID entity, ComponentSerializeContext const & context) const = 0;

    virtual bool read(bfc::SerializedObject const & serialized, EntityID entity, ComponentDeserializeContext const & context) const = 0;

    virtual bool copy(LevelCopyContext * pContext, Level * pDstLevel, EntityID dstEntity, Level const & srcLevel, EntityID srcEntity) const = 0;

    virtual void * addComponent(Level * pDstLevel, EntityID entity) const = 0;
  };

  template<typename T>
  struct LevelComponent_OnCopy {
    inline static void onCopy(LevelCopyContext * pContext, Level * pDstLevel, EntityID dstEntity, Level const & srcLevel, T const & component) {
      BFC_UNUSED(pContext, srcLevel);
      pDstLevel->replace<T>(dstEntity, component);
    }
  };

  /// Specialize this to implement logic before a component is removed from a scene.
  template<typename T>
  struct LevelComponent_OnPreErase {
    inline static void onPreErase(T * pComponent, Level * pLevel) {}
  };

  /// Specialize this to implement logic after a component is added to a scene.
  template<typename T>
  struct LevelComponent_OnPostAdd {
    inline static void onPostAdd(T * pComponent, Level * pLevel) {}
  };

  template<typename T>
  struct LevelComponentHooks
    : LevelComponent_OnCopy<T>
    , LevelComponent_OnPreErase<T>
    , LevelComponent_OnPostAdd<T> {};

  /// Interface used by Levels for a component of type `T`.
  template<typename T>
  class LevelComponentType : public ILevelComponentType {
    virtual bfc::type_index type() const override {
      return bfc::TypeID<T>();
    }

    virtual bfc::SerializedObject write(EntityID entity, ComponentSerializeContext const & context) const override {
      return bfc::serialize(context.pLevel->get<T>(entity), context);
    }

    virtual bool read(bfc::SerializedObject const & serialized, EntityID entity, ComponentDeserializeContext const & context) const override {
      std::optional<T> result = bfc::deserialize<T>(serialized, context);
      if (!result.has_value()) {
        return false;
      }

      context.pLevel->replace<T>(entity, std::move(result.value()));
      return true;
    }

    virtual bool copy(LevelCopyContext * pContext, Level * pDstLevel, EntityID dstEntity, Level const & srcLevel, EntityID srcEntity) const override {
      if (T const * pSrcComponent = srcLevel.tryGet<T>(srcEntity)) {
        LevelComponentHooks<T>::onCopy(pContext, pDstLevel, dstEntity, srcLevel, *pSrcComponent);
        return true;
      }

      return false;
    }

    virtual void * addComponent(Level *pDstLevel, EntityID entity) const override {
      return &pDstLevel->add<T>(entity);
    }
  };

  template<typename T>
  bool registerComponentType(bfc::StringView const & name) {
    return ILevelComponentType::add(name, bfc::NewRef<LevelComponentType<T>>());
  }

  class ILevelComponentStorage;
  namespace impl {
    class ComponentStorageLevelAccess {
      friend Level;
      static void SetOwner(ILevelComponentStorage * pStorage, Level * pLevel);
    };
  }

  class ILevelComponentStorage {
    friend impl::ComponentStorageLevelAccess;
  public:
    ILevelComponentStorage(Level * pOwner);

    virtual ~ILevelComponentStorage() = default;

    virtual bfc::type_index type() const = 0;

    virtual bool exists(EntityID entityID) const = 0;
    virtual bool erase(EntityID entityID)        = 0;

    virtual int64_t size() const     = 0;
    virtual int64_t capacity() const = 0;

    virtual EntityID toEntity(void const * pComponent) const = 0;
    virtual int64_t  toIndex(EntityID entityID) const        = 0;

    virtual bfc::Span<const EntityID> entities() const = 0;

    virtual void * addOpaque(EntityID entityID) = 0;
    virtual void * getOpaque(EntityID entityID) = 0;
    virtual void const * getOpaque(EntityID entityID) const = 0;

    Level * getOwner() const;

  private:
    Level * m_pLevel = nullptr;
  };

  template<typename T>
  class LevelComponentStorage : public ILevelComponentStorage {
  public:
    LevelComponentStorage(Level * pLevel) : ILevelComponentStorage(pLevel) {}

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

      LevelComponentHooks<T>::onPreErase(&m_components[index], getOwner());

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

    virtual void * getOpaque(EntityID entityID) override {
      return tryGet(entityID);
    }

    virtual void const * getOpaque(EntityID entityID) const override {
      return tryGet(entityID);
    }

    virtual void * addOpaque(EntityID entityID) override {
      return &add(entityID);
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

      T * pComponent = &m_components.back();
      LevelComponentHooks<T>::onPostAdd(pComponent, getOwner());
      return *pComponent;
    }

    template<typename... Args>
    T & replace(EntityID entityID, Args &&... args) {
      const int64_t index = toIndex(entityID);

      if (index != bfc::npos) {
        T * pComponent = m_components.begin() + index;
        LevelComponentHooks<T>::onPreErase(pComponent, getOwner());
        bfc::mem::destruct(pComponent);
        bfc::mem::construct(pComponent, std::forward<Args>(args)...);
        LevelComponentHooks<T>::onPostAdd(pComponent, getOwner());

        return *pComponent;
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
