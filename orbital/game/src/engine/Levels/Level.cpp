#include "Level.h"

using namespace bfc;

namespace engine {

  Level::Level() {

  }

  EntityID Level::create() {
    int64_t newEntityIndex   = m_entities.size();
    int64_t newEntityVersion = 1;

    if (m_freed.size() > 0) {
      const EntityID freeID = m_freed.popBack();
      newEntityIndex        = indexOf(freeID);
      newEntityVersion      = versionOf(freeID) + 1;
      m_entities[newEntityIndex] = toEntityID(newEntityIndex, newEntityVersion);
    } else {
      m_entities.pushBack(toEntityID(newEntityIndex, newEntityVersion));
    }

    return m_entities[newEntityIndex];
  }

  bool Level::remove(EntityID const & entityID) {
    const int64_t version = versionOf(entityID);
    const int64_t index   = indexOf(entityID);
    if (!contains(index, version)) {
      return false;
    }

    m_freed.pushBack(m_entities[index]);
    m_entities[index] = InvalidEntity;
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

  bool Level::contains(int64_t const & index, int64_t const & version) const {
    if (version == 0 || index < 0 || index >= m_entities.size()) {
      return false;
    }

    return m_entities[index] == toEntityID(index, version);
  }
}
