#include "LevelSerializer.h"
#include "Assets/AssetManager.h"
#include "Level.h"
#include "util/Log.h"

using namespace bfc;

namespace engine {
  LevelSerializer::LevelSerializer(AssetManager * pManager)
    : m_pManager(pManager) {}

  bool LevelSerializer::serialize(URI const & uri, Level const & level) {
    return m_pManager->getFileSystem()->serialize(uri, serialize(level));
  }

  bool LevelSerializer::deserialize(URI const & uri, Level & level) {
    level.sourceUri = uri;

    std::optional<SerializedObject> serialized = m_pManager->getFileSystem()->deserialize(uri, DataFormat_YAML);
    if (!serialized.has_value()) {
      return false;
    }

    return deserialize(serialized.value(), level);
  }

  SerializedObject LevelSerializer::serialize(Level const & level) {
    ComponentSerializeContext context;
    context.pLevel      = &level;
    context.pSerializer = this;

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
        auto       pInterface    = ILevelComponentType::find(componentName);
        if (pInterface == nullptr) {
          BFC_LOG_WARNING("LevelSerializer", "Unabled to serialized component. Failed to find interface (type=%s). Have you called registerComponentType?",
                          type.name());
          continue;
        }
        components.add(componentName, pInterface->write(entity, context));
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
    bfc::Vector<EntityID>    ids;

    ComponentDeserializeContext context;
    context.pLevel      = &level;
    context.pSerializer = this;

    if (entities.isArray()) {
      for (int64_t i = 0; i < entities.size(); ++i) {
        SerializedObject const & entity = entities.at(i);

        UUID uuid;
        if (!bfc::read(entity.get("uuid"), uuid)) {
          BFC_LOG_WARNING("LevelSerializer", "Failed to deserialized uuid for entity (idx=%lld)", i);
          continue;
        }

        EntityID entityID = level.create(uuid);
        if (entityID == InvalidEntity) {
          entityID = level.find(uuid);
        }

        if (entityID == InvalidEntity) {
          BFC_LOG_WARNING("LevelSerializer", "Failed to create or find entity (uuid=%s)", uuid.toString());
        }
        ids.pushBack(entityID);
      }

      for (int64_t i = 0; i < entities.size(); ++i) {
        SerializedObject const & entity   = entities.at(i);
        auto                     entityID = ids[i];
        if (entityID == InvalidEntity) {
          continue;
        }

        SerializedObject const & components = entity.get("components");
        if (!components.isMap()) {
          continue;
        }

        for (auto & [name, data] : components.asMap()) {
          auto pInterface = ILevelComponentType::find(name);
          if (pInterface == nullptr) {
            BFC_LOG_WARNING("LevelSerializer", "Component type is not supported (type=%s)", name);
            continue;
          }

          if (!pInterface->read(data, entityID, context)) {
            BFC_LOG_WARNING("LevelSerializer", "Failed to read component data (type=%s)", name);
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

    return SerializedObject::MakeMap({{"uri", getAssets()->uriOf(handle).str()}});
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

  EntityID LevelSerializer::readEntityID(bfc::SerializedObject const & serialized, Level const & level) {
    if (serialized.isText()) {
      return level.find(bfc::UUID(serialized.asText()));
    } else {
      return InvalidEntity;
    }
  }

  SerializedObject LevelSerializer::writeEntityID(EntityID const & entityID, Level const & level) {
    if (level.contains(entityID)) {
      return level.uuidOf(entityID).toString();
    } else {
      return SerializedObject::Empty();
    }
  }
} // namespace engine
