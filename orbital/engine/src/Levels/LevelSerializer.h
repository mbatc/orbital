#pragma once

#include "core/SerializedObject.h"
#include "core/Serialize.h"
#include "core/typeindex.h"
#include "util/ThreadPool.h"
#include "Assets/AssetManager.h"
#include "EntityID.h"

namespace bfc {
  class GraphicsDevice;
}

namespace engine {
  class Level;
  class LevelSerializer {
  public:
    class Reader : public SerializerBase<Reader> {
    public:
      Reader(LevelSerializer * pSerializer, Level * pLevel)
        : m_pSerializer(pSerializer)
        , m_pLevel(pLevel)
      {}

      template<typename T>
      bool operator()(SerializedObject const & s, T const & o) {
        return SerializerBase::operator()(s, o);
      }

      template<typename T>
      bool operator()(SerializedObject const &  s, T & o) {
        return SerializerBase::operator()(s, o);
      }

      /// Begin reading a member of a component asynchronously
      template<typename ComponentT, typename MemberT>
      void readAsync(MemberT ComponentT::* pMember, bfc::SerializedObject const & serialized) {
        m_pSerializer->async([this, serialized, pMember, e = entity()]() {
          bfc::Uninitialized<MemberT> value;
          operator()(serialized, value);

          deferRead([pMember, v = value.take(), e](engine::Level & level) {
            if (auto * pComponent = level.tryGet<ComponentT>(e))
              bfc::mem::construct(&(pComponent->*pMember), std::move(v));
          });
        });
      }

      /// Read an asset from `serialized` into `pAsset`.
      template<typename T>
      void readAsset(bfc::SerializedObject const & serialized, bfc::Ref<T> & pAsset) {
        bfc::mem::construct(&pAsset, std::static_pointer_cast<T>(m_pSerializer->readAsset(serialized, bfc::TypeID<T>())));
      }

      /// Defer a read operation until the end of deserialization.
      void deferRead(std::function<void(Level& level)> const & callback) {
        m_pSerializer->deferRead(callback);
      }

      template<typename T>
      bool operator()(bfc::SerializedObject const & serialized, bfc::Ref<T> & pAsset) {
        readAsset<T>(serialized, pAsset);
        return true;
      }

      bool operator()(bfc::SerializedObject const & serialized, engine::EntityID & id) {
        mem::construct(&id, m_pSerializer->readEntityID(serialized, *m_pLevel));
        return true;
      }

      template<typename T>
      bfc::SerializedObject operator()(bfc::Ref<T> const & pAsset) {
        return m_pSerializer->writeAsset(pAsset);
      }

      template<typename T>
      bfc::SerializedObject operator()(engine::EntityID const & id) {
        return m_pSerializer->writeEntityID(id, *m_pWritingLevel);
      }

      EntityID entity() const {
        return m_entity;
      }

      Level & level() const {
        return *m_pLevel;
      }

      void beginEntity(EntityID const & entityId) {
        m_entity = entityId;
      }

    private:
      LevelSerializer * m_pSerializer = nullptr;
      Level *           m_pLevel      = nullptr;
      EntityID          m_entity      = InvalidEntity;
    };

    class Writer : public SerializerBase<Writer> {
    public:
      Writer(LevelSerializer * pSerializer, Level const * pLevel)
        : m_pSerializer(pSerializer)
        , m_pLevel(pLevel) {}

      template<typename T>
      SerializedObject operator()(T const & o) {
        return SerializerBase::operator()(o);
      }

      template<typename T>
      bfc::SerializedObject operator()(bfc::Ref<T> const & pAsset) {
        return m_pSerializer->writeAsset(pAsset);
      }

      template<typename T>
      bfc::SerializedObject operator()(engine::EntityID const & id) {
        return m_pSerializer->writeEntityID(id, *m_pWritingLevel);
      }

      EntityID entity() const {
        return m_entity;
      }

      Level const & level() const {
        return *m_pLevel;
      }

      void beginEntity(EntityID const & entityId) {
        m_entity = entityId;
      }

    private:
      // Read context
      LevelSerializer * m_pSerializer = nullptr;
      Level const *     m_pLevel      = nullptr;
      EntityID          m_entity      = InvalidEntity;
    };

    /// Construct a serializer.
    /// @param pManager The asset manager used to read/write assets.
    LevelSerializer(bfc::ThreadPool * pThreads = &bfc::ThreadPool::Global());

    /// Serialize a level from a URI.
    bool serialize(bfc::URI const & uri, Level const & level);

    /// Deserialize a level to a URI.
    bool deserialize(bfc::URI const & uri, Level & level);

    /// Serialize a level.
    bfc::SerializedObject serialize(Level const & level);

    /// Deserialize a level.
    bool deserialize(bfc::SerializedObject const & serialized, Level & level);

    /// Get the graphics device used by this serializer.
    bfc::GraphicsDevice * getGraphicsDevice() const;

  private:
    /// Defer a read operation until the end of deserialization.
    void deferRead(std::function<void(Level & level)> const & callback);

    /// Serialize an asset pointer.
    bfc::SerializedObject writeAsset(bfc::Ref<void> const & pAsset);

    /// Read an asset from `serialized`.
    bfc::Ref<void> readAsset(bfc::SerializedObject const & serialized, bfc::type_index const & typeinfo);

    /// Read an entity from `serialized`
    static EntityID readEntityID(bfc::SerializedObject const & serialized, Level const & level);

    /// Serialize an entity ID
    static bfc::SerializedObject writeEntityID(EntityID const & entityID, Level const & level);

    template<typename F>
    void async(F && f) {
      std::unique_lock guard{m_lock};
      m_asyncJobs.pushBack(
        m_pThreads->run(bfc::AsyncFlags_AlwaysRun | bfc::AsyncFlags_AllowRunInline, std::forward<F>(f)));
    }

    bfc::ThreadPool * m_pThreads = nullptr;

    std::mutex                                      m_lock;
    bfc::Vector<std::future<void>>                  m_asyncJobs;
    bfc::Vector<std::function<void(Level & level)>> m_deferred;
  };
} // namespace engine
