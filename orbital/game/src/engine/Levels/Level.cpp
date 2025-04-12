#include "Level.h"
#include "platform/Events.h"
#include "LevelSystem.h"

using namespace bfc;

namespace engine {
  Level::Level() 
    : m_pEvents(bfc::NewRef<Events>("Level")) {}

  Level::Level(Level && o) 
    : m_pEvents(o.m_pEvents)
    , m_data(o.m_data)
    , m_entityCount(o.m_entityCount)
    , m_entities(o.m_entities)
    , m_freed(o.m_freed)
    , m_ids(o.m_ids)
    , m_idToEntity(o.m_idToEntity)
    , m_components(o.m_components) {
    for (auto & [type, storage] : m_components)
      impl::ComponentStorageLevelAccess::SetOwner(storage.get(), this);
  }

  Level & Level::operator=(Level && o) {
    std::swap(m_pEvents, o.m_pEvents);
    std::swap(m_data, o.m_data);
    std::swap(m_entityCount, o.m_entityCount);
    std::swap(m_entities, o.m_entities);
    std::swap(m_freed, o.m_freed);
    std::swap(m_ids, o.m_ids);
    std::swap(m_idToEntity, o.m_idToEntity);
    std::swap(m_components, o.m_components);

    for (auto & [type, storage] : m_components)
      impl::ComponentStorageLevelAccess::SetOwner(storage.get(), this);
    for (auto & [type, storage] : o.m_components)
      impl::ComponentStorageLevelAccess::SetOwner(storage.get(), &o);

    return *this;
  }

  Level::~Level() {
    clear();
  }

  EntityID Level::create(std::optional<UUID> const & id) {
    int64_t newEntityIndex   = m_entities.size();
    int64_t newEntityVersion = 1;

    UUID newID = id.has_value() ? id.value() : UUID::New();

    if (m_idToEntity.contains(newID)) {
      return InvalidEntity;
    }

    if (m_freed.size() > 0) {
      const EntityID freeID = m_freed.popBack();
      newEntityIndex        = indexOf(freeID);
      newEntityVersion      = versionOf(freeID) + 1;
      m_entities[newEntityIndex] = toEntityID((uint32_t)newEntityIndex, (uint32_t)newEntityVersion);
      m_ids[newEntityIndex] = newID;
    } else {
      m_entities.pushBack(toEntityID((uint32_t)newEntityIndex, (uint32_t)newEntityVersion));
      m_ids.pushBack(newID);
    }

    m_idToEntity.add(newID, m_entities[newEntityIndex]);

    ++m_entityCount;

    return m_entities[newEntityIndex];
  }

  EntityID Level::find(bfc::UUID const & id) const {
    EntityID ret = InvalidEntity;
    m_idToEntity.tryGet(id, &ret);
    return ret;
  }

  UUID Level::uuidOf(EntityID const & o) const {
    return contains(o) ? m_ids[indexOf(o)] : UUID();
  }

  bool Level::remove(EntityID const & entityID) {
    const int64_t version = versionOf(entityID);
    const int64_t index   = indexOf(entityID);
    if (!contains(index, version)) {
      return false;
    }

    // Remove any components attached to this entity.
    m_idToEntity.erase(m_ids[index]);
    m_ids.erase(index);
    m_entities[index] = InvalidEntity;
    for (auto & [type, pComponents] : m_components) {
      pComponents->erase(entityID);
    }
    m_freed.pushBack(entityID);
    --m_entityCount;
    return true;
  }

  int64_t Level::size() const {
    return m_entityCount;
  }

  int64_t Level::capacity() const {
    return m_entities.size();
  }

  bool Level::contains(EntityID const & entityID) const {
    return contains(indexOf(entityID), versionOf(entityID));
  }

  bfc::Map<bfc::type_index, bfc::Ref<ILevelComponentStorage>> const & Level::components() const {
    return m_components;
  }

  Level::EntityView Level::entities() const {
    return EntityView(this);
  }

  bfc::Vector<EntityID> Level::copyTo(Level * pDstLevel, bfc::Span<EntityID const> const & entities, bool preserveUUIDs) {
    LevelCopyContext context(pDstLevel, this);

    bfc::Vector<EntityID> dstEntities;
    dstEntities.reserve(entities.size());

    // Find or create all the entities
    for (EntityID const & src : entities) {
      if (!contains(src)) {
        dstEntities.pushBack(InvalidEntity);
        continue;
      }

      EntityID dstEntity = InvalidEntity;
      if (preserveUUIDs) {
        bfc::UUID uuid = uuidOf(src);
        dstEntity      = pDstLevel->find(uuid);
        if (dstEntity == InvalidEntity) {
          dstEntity = pDstLevel->create(uuid);
        }
      } else {
        dstEntity = pDstLevel->create();
      }

      context.addMappedEntity(dstEntity, src);
      dstEntities.pushBack(dstEntity);
    }

    for (const auto & [index, src] : enumerate(entities)) {
      EntityID dstEntity = dstEntities[index];
      if (dstEntity == InvalidEntity) {
        continue;
      }

      for (const auto& [type, pComponents] : components()) {
        auto pInterface = ILevelComponentType::find(type);
        pInterface->copy(&context, pDstLevel, dstEntity, *this, src);
      }
    }

    return dstEntities;
  }

  bfc::Vector<EntityID> Level::copyTo(Level * pDstLevel, bool preserveUUIDs) {
    return copyTo(pDstLevel, m_entities);
  }

  bfc::Vector<EntityID> Level::copyTo(Level * pDstLevel, EntityID const & entity, bool preserveUUIDs) {
    bfc::Vector<EntityID> ret = copyTo(pDstLevel, {&entity, 1}, preserveUUIDs);
    ret.eraseValue(InvalidEntity);
    return ret;
  }

  EntityID Level::copy(EntityID const & entity) {
    return copyTo(this, entity, false).front();
  }

  void Level::clear() {
    for (auto & [type, pComponents] : m_components) {
      for (int64_t i = pComponents->entities().size() - 1; i >= 0; --i) {
        pComponents->erase(pComponents->entities()[i]);
      }
    }
    m_ids.clear();
    m_idToEntity.clear();
    m_entities.clear();
    m_freed.clear();
    m_entityCount = 0;
  }

  bool Level::contains(int64_t const & index, int64_t const & version) const {
    if (version == 0 || index < 0 || index >= m_entities.size()) {
      return false;
    }

    return m_entities[index] == toEntityID((uint32_t)index, (uint32_t)version);
  }

  Level::EntityView::Iterator::Iterator(Span<const EntityID> ids, int64_t index)
    : m_ids(ids)
    , m_index(index) {
    nextValidIndex();
  }

  bool Level::EntityView::Iterator::operator==(Iterator const & rhs) const {
    return m_ids.begin() == rhs.m_ids.begin()
      && m_index == rhs.m_index;
  }

  bool Level::EntityView::Iterator::operator!=(Iterator const & rhs) const {
    return !operator==(rhs);
  }

  Level::EntityView::Iterator& Level::EntityView::Iterator::operator++() {
    ++m_index;
    nextValidIndex();
    return *this;
  }

  Level::EntityView::Iterator Level::EntityView::Iterator::operator++(int) {
    Iterator it = *this;

    operator++();
    return it;
  }

  EntityID Level::EntityView::Iterator::operator*() const {
    return m_ids[m_index];
  }

  void Level::EntityView::Iterator::nextValidIndex() {
    while (m_index >= 0 && m_index < m_ids.size() && m_ids[m_index] == InvalidEntity)
      ++m_index;
  }

  Level::EntityView::Iterator Level::EntityView::begin() const {
    return Iterator(m_pLevel->m_entities, 0);
  }

  Level::EntityView::Iterator Level::EntityView::end() const {
    return Iterator(m_pLevel->m_entities, m_pLevel->m_entities.size());
  }

  LevelCopyContext::LevelCopyContext(Level * pDst, Level const * pSrc)
    : m_pDstLevel(pDst)
    , m_pSrcLevel(pSrc) {}

  LevelCopyContext::~LevelCopyContext() {
    for (auto& cb : m_deferred) {
      cb(this, m_pDstLevel);
    }
  }

  EntityID LevelCopyContext::remap(EntityID const & src) const {
    return m_mappedEntities.getOr(src, InvalidEntity);
  }

  bool LevelCopyContext::remap(EntityID * pSrcEntityID) const {
    return m_mappedEntities.tryGet(EntityID(*pSrcEntityID), pSrcEntityID);
  }

  bool LevelCopyContext::addMappedEntity(EntityID const & dst, EntityID const & src) {
    if (!m_pDstLevel->contains(dst) || !m_pSrcLevel->contains(src))
      return false; // invalid entity id
    if (m_mappedEntities.contains(src))
      return false; // already mapped
    return m_mappedEntities.tryAdd(src, dst);
  }

  void LevelCopyContext::defer(std::function<void(LevelCopyContext *, Level *)> const & func) {
    m_deferred.pushBack(func);
  }
} // namespace engine
