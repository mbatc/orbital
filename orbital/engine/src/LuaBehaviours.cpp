#include "LuaBehaviours.h"
#include "core/Map.h"
#include "core/SerializedObject.h"
#include "sol/forward.hpp"
#include "util/Log.h"
#include "util/UUID.h"

namespace engine {
  namespace impl {
    const bfc::String classMetaKey = "_class";

    bool isClassTable(sol::table const & table);

    struct TableSerializer {
      bfc::SerializedObject serialize(sol::table table) {
        bfc::SerializedObject serialization;
        bfc::UUID             uuid = registerTable(table);

        serialization.add("root", uuid.toString());
        bfc::SerializedObject & data = serialization.add("tables", bfc::SerializedObject::MakeMap());
        while (!queue.empty()) {
          auto [uuid, next] = queue.popBack();
          data.add(uuid.toString(), serializeTable(next));
        }

        return serialization;
      }

      bfc::UUID registerTable(sol::table table) {
        const void * ptr = table.pointer();
        if (tables.contains(ptr)) {
          return tables[ptr];
        }

        bfc::UUID newTableID = bfc::UUID::New();
        queue.pushBack(bfc::Pair{newTableID, table});
        tables.add(ptr, newTableID);
        return newTableID;
      }

      // Basic serialization of Lua instances
      // TODO: Don't assume all keys are strings. Any type can be used as an index.
      //       Above changes would also help with this.
      //       Tables will need to be serialized as an array of objects with key and value properties.
      bool isClassTable(sol::table const & table) {
        auto                       mt  = table[sol::metatable_key];
        sol::optional<sol::table> cls = mt[classMetaKey];
        return cls.has_value() && cls->valid();
      }

      bfc::SerializedObject serializeTable(sol::table const & table) {
        if (table == nullSentinel)
          return bfc::SerializedObject::Empty();
        else if (isClassTable(table))
          return serializePublicProperties(table);

        bfc::Map<bfc::String, bfc::SerializedObject> ret;
        table.for_each([&](std::pair<sol::object, sol::object> kvp) {
          auto serialized = serializeLuaObject(kvp.second);
          if (!serialized.has_value())
            return;
          ret[kvp.first.as<bfc::String>()] = serialized.value();
        });
        return bfc::SerializedObject::MakeMap({
          {"kind", "table"},
          {"data", ret},
        });
      }

      bfc::SerializedObject serializeUserType(sol::userdata const & user) {
        BFC_FAIL("Not implemented");
        user.registry_index();
        // TODO: Call native serializer
        bfc::Map<bfc::String, bfc::SerializedObject> ret; // = table["_serialize"](table);
        return bfc::SerializedObject::MakeMap({
          {"kind", "usertype"},
          {"name", "<unknown>"},
          {"data", ret},
        });
      }

      std::optional<bfc::SerializedObject> serializeLuaObject(sol::object const & obj) {
        switch (obj.get_type()) {
        case sol::type::none: return bfc::SerializedObject::Empty();
        case sol::type::lua_nil: return bfc::SerializedObject::Empty();
        case sol::type::string: return obj.as<bfc::String>();
        case sol::type::number: return obj.as<double>();
        case sol::type::boolean: return obj.as<bool>();
        case sol::type::table:
          return bfc::SerializedObject::MakeMap({
            {"table", registerTable(obj.as<sol::table>()).toString()},
          });
        case sol::type::userdata:
          return bfc::SerializedObject::MakeMap({
            {"userdata", registerTable(obj.as<sol::table>()).toString()},
          });
        default: // Not serializable
        case sol::type::lightuserdata:
        case sol::type::thread:
        case sol::type::function: return std::nullopt;
        }
      }

      bfc::SerializedObject serializePublicProperties(sol::table const & instance) {
        auto        mt    = instance[sol::metatable_key];
        sol::object name  = mt[classMetaKey]["name"];
        sol::table  props = mt[classMetaKey]["public"];
        if (!props.valid() || !name.valid()) {
          return bfc::SerializedObject::Empty();
        }

        bfc::Map<bfc::String, bfc::SerializedObject> ret;
        props.for_each([&](std::pair<sol::object, sol::object> kvp) {
          auto prop       = instance[kvp.first];
          auto serialized = serializeLuaObject(prop.get<sol::object>());
          if (!serialized.has_value())
            return;
          ret[kvp.first.as<bfc::String>()] = serialized.value();
        });

        return bfc::SerializedObject::MakeMap({
          {"kind", "class"},
          {"name", name.as<bfc::String>()},
          {"data", ret},
        });
      }

      bfc::Vector<bfc::Pair<bfc::UUID, sol::table>> queue;  // Queue of tables to serialize
      bfc::Map<const void *, bfc::UUID>             tables; // Serialized tables to UUID
      sol::object                                   nullSentinel;
    };

    struct TableDeserializer {
      sol::object                     nullSentinel;
      bfc::Map<bfc::UUID, sol::table> tables; // Deserialized tables by UUID

      bool deserializeInto(lua_State * lua, sol::table table, bfc::SerializedObject const & data) {
        bfc::UUID rootUUID(data.get("root").asText());
        tables[rootUUID] = table;

        // Create all tables
        for (const auto & [uuid, data] : data.get("tables").asMap()) {
          if (!tables.contains(uuid)) {
            tables.add(bfc::UUID(uuid), sol::table(lua, sol::create));
          }
        }

        // Deserialize into tables
        bool ok = true;
        for (const auto & [uuid, data] : data.get("tables").asMap()) {
          ok &= deserializeSingle(lua, tables[uuid], data);
        }
        return ok;
      }

      bool deserializeSingle(lua_State * lua, sol::table table, bfc::SerializedObject const & serialization) {
        bfc::SerializedObject const & kind = serialization.get("kind");
        bfc::SerializedObject const & data = serialization.get("data");

        if (kind == "class") {
          bfc::SerializedObject const & serializedClassName = serialization.get("name");
          return ensureClass(table, serializedClassName.asText()) && deserializeIntoPublicProperties(lua, table, data);
        } else if (kind == "usertype") {
          // TODO: Find in user type registry and invoke deserializer.
          BFC_FAIL("Not implemented");
          return false;
        }

        // Deserialize as table
        if (data.isMap()) {
          for (const auto & [key, value] : data.asMap())
            table[key] = deserializeLuaObject(lua, value);
        }

        return true;
      }

      bool ensureClass(sol::table table, bfc::String const & expectedClass) {
        auto        mt   = table[sol::metatable_key];
        sol::object name = mt[classMetaKey]["name"];
        if (!name.is<bfc::String>() || name.as<bfc::String>() != expectedClass) {
          BFC_FAIL("Constructing class during deserialization is not supported yet");
          return false;
        }
        return true;
      }

      bool deserializeIntoPublicProperties(lua_State * lua, sol::table instance, bfc::SerializedObject const & data) {

        auto       mt    = instance[sol::metatable_key];
        sol::table props = mt[classMetaKey]["public"];
        if (!props.valid()) {
          return false;
        }

        props.for_each([&](std::pair<sol::object, sol::object> kvp) {
          auto const name     = kvp.first.as<bfc::String>();
          if (data.has(name)) {
            instance[kvp.first] = deserializeLuaObject(instance.lua_state(), data.get(name));
          }
        });
        return true;
      }

      sol::object deserializeLuaObject(lua_State * lua, bfc::SerializedObject const & serialization) {
        switch (serialization.getType()) {
        case bfc::SerializedObject::Type_Empty: return nullSentinel;
        case bfc::SerializedObject::Type_Int: return sol::make_object(lua, serialization.asInt());
        case bfc::SerializedObject::Type_Float: return sol::make_object(lua, serialization.asFloat());
        case bfc::SerializedObject::Type_Text: return sol::make_object(lua, serialization.asText());
        case bfc::SerializedObject::Type_Map: return deserializeReference(lua, serialization);
        default: return nullptr;
        }
      }

      sol::table deserializeReference(lua_State * lua, bfc::SerializedObject const & serialization) {
        bfc::SerializedObject const & table = serialization.get("table");
        if (table.isText())
          return findTable(bfc::UUID(table.asText()));
        // bfc::SerializedObject const & userType = serialization.get("usertype");
        return nullptr;
      }

      sol::object findTable(bfc::UUID const & uuid) {
        if (tables.contains(uuid))
          return tables[uuid];
        return nullptr;
      }
    };
  } // namespace impl

  LuaBehaviourInstance::LuaBehaviourInstance(LuaBehaviourClass * pClass, sol::table instance)
    : m_pClass(pClass)
    , m_instance(instance)
    , m_update(instance["update"])
    , m_activate(instance["activate"])
    , m_deactivate(instance["deactivate"])
    , m_pause(instance["pause"])
    , m_stop(instance["stop"])
    , m_destroyed(instance["destroyed"]) {}

  void LuaBehaviourInstance::activate() {
    if (m_activate)
      m_activate(m_instance);
  }

  void LuaBehaviourInstance::deactivate() {
    if (m_deactivate)
      m_deactivate(m_instance);
  }

  void LuaBehaviourInstance::update(bfc::Timestamp dt) {
    if (m_update) {
      sol::protected_function_result result = m_update(m_instance, dt.secs());
      if (!result.valid()) {
        sol::error err = result;
        BFC_LOG_ERROR("lua", "Function error: %s", err.what());
      }
    }
  }

  void LuaBehaviourInstance::stop() {
    if (m_stop)
      m_stop(m_instance);
  }

  void LuaBehaviourInstance::pause() {
    if (m_pause)
      m_pause(m_instance);
  }

  void LuaBehaviourInstance::destroyed() {
    if (m_destroyed)
      m_destroyed(m_instance);
  }

  bool LuaBehaviourInstance::bind(engine::Level * pLevel, engine::EntityID entityID) {
    return m_pClass->bindEntity(m_instance, pLevel, entityID);
  }

  bool LuaBehaviourInstance::unbind() {
    return m_pClass->unbindEntity(m_instance);
  }

  BehaviourClass * LuaBehaviourInstance::getClass() const {
    return m_pClass;
  }

  LuaBehaviourClass::LuaBehaviourClass(sol::state * pLua, sol::table classTable, bfc::Ref<Shared> pShared) {
    m_pShared = pShared;
    m_class   = classTable;
    m_new     = m_class.as<sol::protected_function>();
  }

  bfc::Ref<BehaviourInstance> LuaBehaviourClass::instantiate() {
    auto result = m_new();
    if (!result.valid()) {
      sol::error err = result;
      BFC_LOG_ERROR("lua", "Function error: %s", err.what());
      return nullptr;
    }

    return bfc::NewRef<LuaBehaviourInstance>(this, result.get<sol::table>());
  }

  bfc::SerializedObject LuaBehaviourClass::serialize(BehaviourInstance * pSerialize) {
    auto pBehaviour = static_cast<LuaBehaviourInstance *>(pSerialize);

    impl::TableSerializer serializer;
    serializer.nullSentinel = m_pShared->nullValue;
    return serializer.serialize(pBehaviour->m_instance);
  }

  bfc::Ref<BehaviourInstance> LuaBehaviourClass::deserialize(bfc::SerializedObject const & serialization) {
    auto pBehaviour = std::static_pointer_cast<LuaBehaviourInstance>(instantiate());

    impl::TableDeserializer deserializer;
    deserializer.nullSentinel = m_pShared->nullValue;
    if (!deserializer.deserializeInto(pBehaviour->m_instance.lua_state(), pBehaviour->m_instance, serialization)) {
      return nullptr;
    }

    return pBehaviour;
  }

  bool LuaBehaviourClass::bindEntity(sol::table table, engine::Level * pLevel, engine::EntityID entity) {
    return m_pShared->bind(table, pLevel, entity);
  }

  bool LuaBehaviourClass::unbindEntity(sol::table table) {
    return m_pShared->unbind(table);
  }
} // namespace engine
