#pragma once

#include "core/SerializedObject.h"
#include "core/Serialize.h"
#include "core/typeindex.h"
#include "util/ThreadPool.h"
#include "Assets/AssetManager.h"

namespace engine {
  using EntityID = uint64_t;

  class Level;
  class LevelSerializer;
  class AssetManager;

  /// Context that can be specified in Serializer<T>::write to access level serializer.
  struct ComponentSerializeContext : AssetSerializerContext {
    Level const *     pLevel;
    LevelSerializer * pSerializer;
    EntityID          entity;
  };

  /// Context that can be specified in Serializer<T>::read to access level serializer.
  struct ComponentDeserializeContext : AssetSerializerContext {
    Level * pLevel;
    LevelSerializer * pSerializer;
    EntityID          entity;
  };

  class LevelSerializer {
  public:
    /// Construct a serializer.
    /// @param pManager The asset manager used to read/write assets.
    LevelSerializer(AssetManager * pManager, bfc::ThreadPool * pThreads = &bfc::ThreadPool::Global());

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

    /// Begin reading a member of a component asynchronously
    template<typename ComponentT, typename MemberT>
    void readAsync(ComponentDeserializeContext const & ctx, MemberT ComponentT::* pMember, bfc::SerializedObject const & serialized) {
      std::unique_lock guard{ m_lock };
      m_asyncJobs.pushBack(m_pThreads->run(bfc::AsyncFlags_AlwaysRun | bfc::AsyncFlags_AllowRunInline, [serialized, ctx, pMember]() {
        bfc::Uninitialized<MemberT> value;
        serialized.read(value, ctx);

        ctx.pSerializer->deferRead([ctx, pMember, v = std::move(value.take())](engine::Level & level) {
          if (auto * pComponent = level.tryGet<ComponentT>(ctx.entity))
            bfc::mem::construct(&(pComponent->*pMember), std::move(v));
        });
      }));
    }

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
    bfc::ThreadPool * m_pThreads = nullptr;

    std::mutex                                      m_lock;
    bfc::Vector<std::future<void>>                  m_asyncJobs;
    bfc::Vector<std::function<void(Level & level)>> m_deferred;
  };

} // namespace engine

namespace bfc {
  template<typename T>
  struct Serializer<Ref<T>> {
    inline static SerializedObject write(Ref<T> const & o, engine::ComponentSerializeContext const & ctx) {
      return ctx.pSerializer->writeAsset(o);
    }

    inline static bool read(SerializedObject const & s, Ref<T> & o, engine::ComponentDeserializeContext const & ctx) {
      ctx.pSerializer->readAsset(s, o);
      return true;
    }
  };
}
