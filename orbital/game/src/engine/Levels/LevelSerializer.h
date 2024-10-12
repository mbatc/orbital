#pragma once

#include "core/SerializedObject.h"

namespace engine {
  class Level;
  class AssetManager;

  class LevelSerializer {
  public:
    LevelSerializer(AssetManager * pManager);

    bool serialize(bfc::URI const & uri, Level const & level);
    bool deserialize(bfc::URI const & uri, Level & level);

    bfc::SerializedObject serialize(Level const & level);
    bool deserialize(bfc::SerializedObject const & serialized, Level & level);

    AssetManager * getAssets() const;

  private:
    AssetManager * m_pManager = nullptr;
  };
}
