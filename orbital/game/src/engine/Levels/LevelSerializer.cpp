#include "LevelSerializer.h"
#include "Level.h"
#include "../assets/AssetManager.h"
#include "util/Log.h"

using namespace bfc;

namespace engine {
  LevelSerializer::LevelSerializer(AssetManager * pManager)
    : m_pManager(pManager) {}

  bool LevelSerializer::serialize(URI const & uri, Level const & level) {
    return bfc::serialize(uri, serialize(level));
  }

  bool LevelSerializer::deserialize(URI const & uri, Level & level) {
    std::optional<SerializedObject> serialized = bfc::deserialize(uri, DataFormat_YAML);
    if (!serialized.has_value()) {
      return false;
    }

    return deserialize(serialized.value(), level);
  }

  SerializedObject LevelSerializer::serialize(Level const & level) {
    SerializedObject entityList = SerializedObject::MakeArray();
    for (EntityID entity : level.entities()) {
      SerializedObject serializedEntity = SerializedObject::MakeMap();
      serializedEntity.add("uuid", bfc::serialize(level.uuidOf(entity)));

      SerializedObject components = SerializedObject::MakeMap();
      for (auto & [type, pStorage] : level.components()) {
        if (!pStorage->exists(entity)) {
          continue;
        }

        StringView componentName = ILevelComponentType::findName(type);
        auto            pInterface    = ILevelComponentType::find(componentName);
        if (pInterface == nullptr) {
          BFC_LOG_WARNING("LevelSerializer", "Unabled to serialized component. Failed to find interface (type=%s). Have you called registerComponentType?",
                          type.name());
          continue;
        }
        components.add(componentName, pInterface->write(this, level, entity));
      }

      serializedEntity.add("components") = std::move(components);
      entityList.asArray().pushBack(std::move(serializedEntity));
    }

    return SerializedObject::MakeMap({
      {"entities", std::move(entityList)},
    });
  }

  bool LevelSerializer::deserialize(SerializedObject const & serialized, Level & level) {
    SerializedObject const & entities = serialized.get("entities");
    if (entities.isArray()) {
      for (int64_t i = 0; i < entities.size(); ++i) {
        SerializedObject const & entity = entities.at(i);

        UUID uuid;
        if (!bfc::deserialize(entity.get("uuid"), uuid)) {
          BFC_LOG_WARNING("LevelSerializer", "Failed to deserialized uuid for entity (idx=%lld)", i);
          continue;
        }

        EntityID entityID = level.create(uuid);
        if (entityID == InvalidEntity) {
          entityID = level.find(uuid);
        }

        if (entityID == InvalidEntity) {
          BFC_LOG_WARNING("LevelSerializer", "Failed to create or find entity (uuid=%s)", uuid.toString());
          continue;
        }

        SerializedObject const & components = entity.get("components");
        if (!components.isMap()) {
          continue;
        }

        for (auto & [name, data] : components.asMap()) {
          auto pInterface = ILevelComponentType::find(name);
          if (pInterface == nullptr) {
            BFC_LOG_WARNING("LevelSerializer", "Component type is not supported (entity-uuid=%s, type=%s)", uuid.toString(), name);
            continue;
          }

          if (!pInterface->read(this, data, level, entityID)) {
            BFC_LOG_WARNING("LevelSerializer", "Failed to read component data (entity-uuid=%s, type=%s)", uuid.toString(), name);
            continue;
          }
        }
      }
    }

    // Run deferred reads
    for (auto & cb : m_deferred) {
      cb(level);
    }

    m_deferred.clear();

    return true;
  }

  void LevelSerializer::deferRead(std::function<void(Level & level)> const & callback) {
    m_deferred.pushBack(callback);
  }

  AssetManager * LevelSerializer::getAssets() const {
    return m_pManager;
  }

  SerializedObject LevelSerializer::writeAsset(Ref<void> const & pAsset) {
    AssetHandle handle = getAssets()->find(pAsset);
    if (handle == InvalidAssetHandle) {
      return SerializedObject::Empty();
    }

    return SerializedObject::MakeMap({
      { "uri", getAssets()->uriOf(handle).str() }
    });
  }

  Ref<void> LevelSerializer::readAsset(SerializedObject const & serialized, type_index const & typeInfo) {
    SerializedObject const & uriItem = serialized.get("uri");
    if (uriItem.isText()) {
      AssetManager * pAssets = getAssets();
      bfc::URI       uri     = uriItem.asText();
      return pAssets->load(pAssets->add(uri, typeInfo), typeInfo);
    }

    SerializedObject const & idItem = serialized.get("uuid");
    if (idItem.isText()) {
      AssetManager * pAssets = getAssets();
      bfc::UUID      uuid    = uriItem.asText();
      return pAssets->load(pAssets->find(uuid), typeInfo);
    }

    return nullptr;
  }
} // namespace engine
