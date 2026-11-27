#pragma once

#include "Level.h"

namespace engine {
  class LevelTemplate {
  public:
    LevelTemplate() = default;

    LevelTemplate(SerializedObject data) {
      m_serialized = data;
    }

    EntityID create(Level *pDst) {
      Level instance;
      LevelSerializer().deserialize(m_serialized, instance);

      Vector<EntityID> ids = instance.copyTo(pDst, false);
      return ids.front();
    }

    void set(Level const & l) {
      m_serialized = LevelSerializer().serialize(l);
    }

  private:
    bfc::SerializedObject m_serialized;
  };
}
