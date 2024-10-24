#pragma once

#include "core/SerializedObject.h"
#include "core/Serialize.h"
#include "core/typeindex.h"

namespace engine {
  using EntityID = uint64_t;

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

    inline static bool read(LevelSerializer * pSerializer, bfc::SerializedObject const & serialized, Level & level, EntityID entity, T & o) {
      BFC_UNUSED(pSerializer, level);

      return serialized.read(o);
    }
  };

  class LevelSerializer {
  public:
    /// Construct a serializer.
    /// @param pManager The asset manager used to read/write assets.
    LevelSerializer(AssetManager * pManager);

    /// Serialize a level from a URI.
    bool serialize(bfc::URI const & uri, Level const & level);

    /// Deserialize a level to a URI.
    bool deserialize(bfc::URI const & uri, Level & level);

    /// Serialize a level.
    bfc::SerializedObject serialize(Level const & level);

    /// Deserialize a level.
    bool deserialize(bfc::SerializedObject const & serialized, Level & level);

    /// Defer a read operation until the end of deserialization.
    void deferRead(std::function<void(Level & level)> const & callback);

    /// Get the asset manager used by this serializer.
    AssetManager * getAssets() const;

    /// Serialize an asset pointer.
    bfc::SerializedObject writeAsset(bfc::Ref<void> const & pAsset);

    /// Serialize a iterable containing asset pointers.
    template<typename Iterable>
    bfc::SerializedObject writeAssets(Iterable const & assets) {
      bfc::SerializedObject ret = bfc::SerializedObject::MakeArray();
      for (const auto & asset : assets) {
        ret.pushBack(writeAsset(asset));
      }
      return ret;
    }

    /// Read an asset from `serialized`.
    bfc::Ref<void> readAsset(bfc::SerializedObject const & serialized, bfc::type_index const & typeinfo);

    /// Read an entity from `serialized`
    static EntityID readEntityID(bfc::SerializedObject const & serialized, Level const & level);

    /// Serialize an entity ID
    static bfc::SerializedObject writeEntityID(EntityID const & entityID, Level const & level);

    /// Read an asset from `serialized` into `pAsset`.
    template<typename T>
    void readAsset(bfc::SerializedObject const & serialized, bfc::Ref<T> & pAsset) {
      bfc::mem::construct(&pAsset, std::static_pointer_cast<T>(readAsset(serialized, bfc::TypeID<T>())));
    }

    /// Read an asset from `serialized` into `assets`.
    /// `assets` should be an uninitialized Vector as this function will construct it.
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
