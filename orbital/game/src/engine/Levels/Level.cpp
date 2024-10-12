#include "Level.h"

using namespace bfc;

namespace engine {
  Level::Level() {}

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
    m_freed.pushBack(m_entities[index]);
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
} // namespace engine
