#pragma once

#include "core/SerializedObject.h"
#include "core/typeindex.h"

namespace engine {
  class Level;
  class LevelSerializer;
  class AssetManager;

  // A Specific serializer type that should be specialized for more control over entity component serialization.
  template<typename T>
  struct LevelComponentSerializer {
    // Default implementation delegates to a bfc serialize implementation for the component type.
    inline static bfc::SerializedObject write(LevelSerializer * pSerializer, Level const & level, T const & component) {
      BFC_UNUSED(pSerializer, level);

      return bfc::serialize(component);
    }

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & serialized, Level & level, T & o) {
      BFC_UNUSED(pSerializer, level);

      return serialized.read(o);
    }
  };

  class LevelSerializer {
  public:
    LevelSerializer(AssetManager * pManager);

    bool serialize(bfc::URI const & uri, Level const & level);
    bool deserialize(bfc::URI const & uri, Level & level);

    bfc::SerializedObject serialize(Level const & level);
    bool                  deserialize(bfc::SerializedObject const & serialized, Level & level);

    void deferRead(std::function<void(Level & level)> const & callback);

    AssetManager * getAssets() const;

    bfc::SerializedObject writeAsset(bfc::Ref<void> const & pAsset);

    template<typename Iterable>
    bfc::SerializedObject writeAssets(Iterable const & assets) {
      bfc::SerializedObject ret = bfc::SerializedObject::MakeArray();
      for (const auto & asset : assets) {
        ret.pushBack(writeAsset(asset));
      }
      return ret;
    }

    bfc::Ref<void> readAsset(bfc::SerializedObject const & serialized, bfc::type_index const & typeinfo);

    template<typename T>
    void readAsset(bfc::SerializedObject const & serialized, bfc::Ref<T> & pAsset) {
      bfc::mem::construct(&pAsset, std::static_pointer_cast<T>(readAsset(serialized, bfc::TypeID<T>())));
    }

    template<typename T>
    void readAssets(bfc::SerializedObject const & serialized, bfc::Vector<bfc::Ref<T>> & assets) {
      bfc::mem::construct(&assets);
      if (!serialized.isArray()) {
        return;
      }

      assets.reserve(serialized.size());
      for (bfc::SerializedObject const & item : serialized.asArray()) {
        assets.pushBack(std::static_pointer_cast<T>(readAsset(item, bfc::TypeID<T>())));
      }
    }

  private:
    AssetManager * m_pManager = nullptr;

    bfc::Vector<std::function<void(Level & level)>> m_deferred;
  };
} // namespace engine
