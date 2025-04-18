#pragma once

#include "Filename.h"
#include "SerializedObject.h"
#include "Stream.h"
#include "URI.h"
#include "../math/MathTypes.h"
#include "../util/Scan.h"
#include "../util/UUID.h"
#include "Reflect.h"

namespace bfc {
  template<typename T, typename Context = DefaultSerializerContext>
  SerializedObject serialize(T const & o, Context const & ctx = {}) {
    SerializedObject serialized;
    serialized.write(o, ctx);
    return serialized;
  }

  template<typename T, typename Context = DefaultSerializerContext>
  void write(SerializedObject & dst, T const & o, Context const & ctx = {}) {
    return dst.write(o, ctx);
  }

  template<typename T, typename Context = DefaultSerializerContext>
  bool read(SerializedObject const & serialized, T & o, Context const & ctx = {}) {
    Uninitialized<T> buf;
    if (!serialized.read(buf.get())) {
      return false;
    }
    o = buf.take();
    return true;
  }

  template<typename T, typename Context = DefaultSerializerContext>
  std::optional<T> deserialize(SerializedObject const & serialized, Context const & ctx = {}) {
    Uninitialized<T> buf;
    if (!serialized.read(buf.get(), ctx)) {
      return std::nullopt;
    }
    return buf.take();
  }

  enum DataFormat {
    DataFormat_YAML,
    DataFormat_Binary,
    DataFormat_Count,
  };

  BFC_API bool serialize(URI const & uri, SerializedObject const & o, DataFormat const & fmt = DataFormat_YAML);

  BFC_API std::optional<SerializedObject> deserialize(URI const & uri, DataFormat const & fmt = DataFormat_YAML);

  template<typename T, typename Context = DefaultSerializerContext>
  bool serialize(URI const & uri, T const & o, DataFormat const & fmt = DataFormat_YAML, Context const & ctx = {}) {
    return serialize(uri, serialize<T>(o), fmt);
  }

  template<typename T, typename Context = DefaultSerializerContext>
  std::optional<T> deserialize(URI const & uri, DataFormat const & fmt = DataFormat_YAML, Context const & ctx = {}) {
    Uninitialized<T> buf;
    std::optional<SerializedObject> serialized = deserialize(uri, fmt);
    if (!serialized) {
      return {};
    }
    if (!serialized->read(buf.get())) {
      return {};
    }
    return std::move(buf.get());
  }

  /// EnumValueMap can be specialized to map enums of type 'T' to an arbitrary value of any type
  template<typename T>
  struct EnumValueMap {
    // inline static Vector<any> const mapping;
    // inline static Map<T, any> const mapping;
  };
  BFC_DEFINE_MEMBER_CHECK(mapping);
  BFC_DEFINE_MEMBER_CHECK(__testIsDefaultSerializer);

  /// Test if a type has a custom serializer implemented.
  template<typename T>
  inline static constexpr bool has_custom_serializer_v = !BFC_HAS_MEMBER(Serializer<T>, __testIsDefaultSerializer);

  template<typename T>
  struct Serializer {
  private:
    template<int64_t I, typename Context, typename... Members>
    static bool writeMember(SerializedObject &dst, T const & o, Context const & ctx, Reflection<T, Members...> const & reflected) {
      if constexpr (reflected.isMember<I>()) {
        bfc::write(dst.get(reflected.name<I>()), reflected.get<I>(&o), ctx);
        return true;
      } else {
        return false;
      }
    }

    template<typename Context, typename... Members, int64_t... Indices>
    static SerializedObject writeMembers(T const & o, Context const & ctx, Reflection<T, Members...> const & reflection,
                                         std::integer_sequence<int64_t, Indices...>) {
      SerializedObject ret;
      (writeMember<Indices>(ret, o, ctx, reflection), ...);
      return ret;
    }

    template<int64_t I, typename Context, typename... Members>
    static bool readMember(SerializedObject const & src, T & o, Context const & ctx, Reflection<T, Members...> const & reflected) {
      if constexpr (reflected.isMember<I>()) {
        src.get(reflected.name<I>()).read(reflected.get<I>(&o), ctx);
        return true;
      } else {
        return false;
      }
    }

    template<typename Context, typename... Members, int64_t... Indices>
    static void readMembers(SerializedObject const & src, T & o, Context const & ctx, Reflection<T, Members...> const & reflection,
                                        std::integer_sequence<int64_t, Indices...>) {
      (readMember<Indices>(src, o, ctx, reflection), ...);
    }

  public:
    static inline constexpr bool __testIsDefaultSerializer = true;

    template<typename Context, typename U = T, std::enable_if_t<!(std::is_floating_point_v<U> || std::is_integral_v<U> || std::is_enum_v<U>)> * = 0>
    static SerializedObject write(T const & o, Context const & ctx) {
      if constexpr (bfc::has_reflect_v<T>) {
        auto reflection = reflect<T>();
        return writeMembers(o, ctx, reflection, std::make_integer_sequence<int64_t, reflection.size()>{});
      } else {
        static_assert(false, "write() is not implemented for type T. To enable serialization implement a Serializer or the `reflect<T>` function.");
      }
    }

    template<typename Context, typename U = T, std::enable_if_t<!(std::is_floating_point_v<U> || std::is_integral_v<U> || std::is_enum_v<U>)> * = 0>
    static bool read(SerializedObject const & s, T & o, Context const & ctx) {
      if constexpr (bfc::has_reflect_v<T>) {
        memset(&o, 0, sizeof(T));
        auto reflection = reflect<T>();
        readMembers(s, o, ctx, reflection, std::make_integer_sequence<int64_t, reflection.size()>{});
        return true;
      } else {
        static_assert(false, "read() is not implemented for type T. To enable serialization implement a Serializer or the `reflect<T>` function.");
      }
    }

    template<typename Context, typename U = T, std::enable_if_t<std::is_integral_v<U> || std::is_enum_v<U>> * = 0>
    static SerializedObject write(T const & o, Context const & ctx) {
      if constexpr (BFC_HAS_MEMBER(EnumValueMap<T>, mapping)) {
        auto & mapping = EnumValueMap<T>::mapping;
        using MappingT = decltype(EnumValueMap<T>::mapping);

        if constexpr (is_vector_v<MappingT>) {
          // Serialize mapped values
          if (o < 0 || o >= mapping.size()) {
            return SerializedObject::Empty();
          } else {
            return serialize(mapping[o]);
          }
        } else if constexpr (is_map_v<MappingT>) {
          static_assert(std::is_same_v<MappingT::KeyType, T>, "EnumValueMap<T>::mapping keys must be of type 'T'");
          auto * pValue = mapping.tryGet(o);
          if (pValue == nullptr) {
            return SerializedObject::Empty();
          } else {
            return serialize(*pValue);
          }
        } else {
          static_assert(false, "EnumValueMap<T>::mapping is not a supported type. Must be Vector<any> or Map<T, any>");
        }
      } else {
        // Just serialize as an integer
        return SerializedObject::MakeInt((int64_t)o);
      }
    }

    template<typename Context, typename U = T, std::enable_if_t<std::is_integral_v<U> || std::is_enum_v<U>> * = 0>
    static bool read(SerializedObject const & s, T & o, Context const & ctx) {
      if constexpr (BFC_HAS_MEMBER(EnumValueMap<T>, mapping)) {
        auto & mapping = EnumValueMap<T>::mapping;
        using MappingT = decltype(EnumValueMap<T>::mapping);

        if constexpr (is_vector_v<MappingT>) {
          // Read value and find mapping in Vector
          using ElementType = MappingT::ElementType;
          Uninitialized<ElementType> buffer;
          if (!s.read(buffer.get(), ctx)) {
            return false;
          }

          bool success = false;
          for (int64_t i = 0; i < mapping.size(); ++i) {
            if (buffer.get() == mapping[i]) {
              o = (T)i;
              success = true;
              break;
            }
          }
          buffer.destruct();
          return success;
        } else if constexpr (is_map_v<MappingT>) {
          static_assert(std::is_same_v<MappingT::KeyType, T>, "EnumValueMap<T>::mapping keys must be of type 'T'");
          // Read value and find mapping in Map
          using ElementType = MappingT::ValueType;
          Uninitialized<ElementType> buffer;
          if (!s.read(buffer.get(), ctx)) {
            return false;
          }

          bool success = false;
          for (auto& [key, value] : mapping) {
            if (value == buffer.get()) {
              o       = (T)key;
              success = true;
              break;
            }
          }
          buffer.destruct();
          return success;
        } else {
          static_assert(false, "EnumValueMap<T>::mapping is not a supported type. Must be Vector<any> or Map<T, any>");
        }
      } else {
        // Read as integer
        switch (s.getType()) {
        case SerializedObject::Type_Float: o = (T)s.asFloat(); break;
        case SerializedObject::Type_Int: o = (T)s.asInt(); break;
        case SerializedObject::Type_Text: {
          int64_t len = 0;
          o           = (T)Scan::readInt(s.asText(), &len);
          if (len == 0) {
            return false;
          }
        } break;
        default: return false;
        }
      }

      return true;
    }

    template<typename Context, typename U = T, std::enable_if_t<std::is_floating_point_v<U>> * = 0>
    static SerializedObject write(T const & o, Context const &) {
      return SerializedObject::MakeFloat((double)o);
    }

    template<typename Context, typename U = T, std::enable_if_t<std::is_floating_point_v<U>> * = 0>
    static bool read(SerializedObject const & s, T & o, Context const &) {
      switch (s.getType()) {
      case SerializedObject::Type_Float: o = (T)s.asFloat(); break;
      case SerializedObject::Type_Int: o = (T)s.asInt(); break;
      case SerializedObject::Type_Text: {
        int64_t len = 0;
        o           = (T)Scan::readFloat(s.asText(), &len);
        if (len == 0) {
          return false;
        }
      }
      default: return false;
      }

      return true;
    }
  };

  template<typename T>
  struct Serializer<Uninitialized<T>> {
    template<typename Context>
    static SerializedObject write(Uninitialized<T> const & o, Context const & ctx) {
      return serialize(o.get(), ctx);
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Uninitialized<T> & o, Context const & ctx) {
      return deserialize(o.get(), ctx);
    }
  };

  template<typename T>
  struct Serializer<std::optional<T>> {
    template<typename Context>
    static SerializedObject write(std::optional<T> const & o, Context const & ctx) {
      if (o.has_value()) {
        return serialize(o.value(), ctx);
      } else {
        return SerializedObject::Empty();
      }
    }

    template<typename Context>
    static bool read(SerializedObject const & s, std::optional<T> & o, Context const & ctx) {
      if (s.isEmpty()) {
        mem::construct(&o, std::nullopt);
        return true;
      }

      Uninitialized<T> value;
      if (!s.read(value.get(), ctx)) {
        return false;
      }

      o.emplace(value.take());
      return true;
    }
  };

  template<typename T>
  struct Serializer<Vector2<T>> {
    template<typename Context>
    static SerializedObject write(Vector2<T> const & o, Context const & ctx) {
      return SerializedObject::MakeArray({serialize(o.x, ctx), serialize(o.y, ctx)});
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Vector2<T> & o, Context const & ctx) {
      bool success = true;
      for (int i = 0; i < 2; ++i) {
        success &= s.at(i).read(o[i], ctx);
      }
      return success;
    }
  };

  template<typename T>
  struct Serializer<Vector3<T>> {
    template<typename Context>
    static SerializedObject write(Vector3<T> const & o, Context const & ctx) {
      return SerializedObject::MakeArray({serialize(o.x, ctx), serialize(o.y, ctx), serialize(o.z, ctx)});
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Vector3<T> & o, Context const & ctx) {
      bool success = true;
      for (int i = 0; i < 3; ++i) {
        success &= s.at(i).read(o[i], ctx);
      }
      return success;
    }
  };

  template<typename T>
  struct Serializer<Vector4<T>> {
    template<typename Context>
    static SerializedObject write(Vector4<T> const & o, Context const & ctx) {
      return SerializedObject::MakeArray({serialize(o.x, ctx), serialize(o.y, ctx), serialize(o.z, ctx), serialize(o.w, ctx)});
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Vector4<T> & o, Context const & ctx) {
      bool success = true;
      for (int i = 0; i < 4; ++i) {
        success &= s.at(i).read(o[i], ctx);
      }
      return success;
    }
  };

  template<typename T>
  struct Serializer<Quaternion<T>> {
    template<typename Context>
    static SerializedObject write(Quaternion<T> const & o, Context const & ctx) {
      return SerializedObject::MakeArray({serialize(o.x, ctx), serialize(o.y, ctx), serialize(o.z, ctx), serialize(o.w, ctx)});
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Quaternion<T> & o, Context const & ctx) {
      bool success = true;
      for (int i = 0; i < 4; ++i) {
        success &= s.at(i).read(o[i], ctx);
      }
      return success;
    }
  };

  template<typename T>
  struct Serializer<Matrix<T>> {
    template<typename Context>
    static SerializedObject write(Matrix<T> const & o, Context const & ctx) {
      return SerializedObject::MakeArray({
        SerializedObject(o[0], ctx),
        SerializedObject(o[1], ctx),
        SerializedObject(o[2], ctx),
        SerializedObject(o[3], ctx)
        });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Matrix<T> & o, Context const & ctx) {
      bool success = true;
      for (int i = 0; i < 4; ++i) {
        success &= s.at(i).read(o[i], ctx);
      }
      return success;
    }
  };

  template<typename T>
  struct Serializer<Vector<T>> {
    template<typename Context>
    static SerializedObject write(Vector<T> const & o, Context const & ctx) {
      return serialize(o.getView(), ctx);
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Vector<T> & o, Context const & ctx) {
      mem::construct(&o);

      if (!s.isArray()) {
        return s.isEmpty();
      }

      o.reserve(s.size());
      for (SerializedObject const & elm : s.asArray()) {
        Uninitialized<T> val;
        if (!elm.read(val.get(), ctx)) {
          return false;
        }
        o.pushBack(std::move(val.get()));
      }

      return true;
    }
  };

  template<typename T>
  struct Serializer<Span<T>> {
    template<typename Context>
    static SerializedObject write(Span<T> const & o, Context const & ctx) {
      SerializedObject s = SerializedObject::MakeArray();
      s.asArray().resize(o.size());
      for (auto & [i, item] : enumerate(o)) {
        s.asArray()[i].write(item, ctx);
      }
      return s;
    }
  };

  template<typename Key, typename Value>
  struct Serializer<Pair<Key, Value>> {
    static inline String firstName  = "first";
    static inline String secondName = "second";

    template<typename Context>
    static SerializedObject write(Pair<Key, Value> const & o, Context const & ctx) {
      return SerializedObject::MakeMap({
        { firstName, serialize(o.first, ctx)},
        { secondName, serialize(o.second, ctx)}
        });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Pair<Key, Value> & o, Context const & ctx) {
      bool success = elm[firstName].read(o.first, ctx);
      success &= elm[secondName].read(o.second, ctx);
      return success;
    }
  };

  template<typename Key, typename Value>
  struct Serializer<Map<Key, Value>> {
    static inline String keyName = "key";
    static inline String valName = "value";

    template<typename Context>
    static SerializedObject write(Map<Key, Value> const & o, Context const & ctx) {
      SerializedObject s = SerializedObject::MakeArray();
      for (auto const & [key, value] : o) {
        s.pushBack(SerializedObject::MakeMap({{keyName, serialize(key, ctx)}, {valName, serialize(value, ctx)}}));
      }
      return s;
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Map<Key, Value> & o, Context const & ctx) {
      mem::construct(&o);
      if (!s.isArray()) {
        return s.isEmpty();
      }

      for (SerializedObject const & elm : s.asArray()) {
        if (!elm.isMap() && elm.size() != 2) {
          return false;
        }

        Key   key;
        Value val;
        if (!(elm.get(keyName).read(key, ctx) && elm.get(valName).read(val, ctx))) {
          return false;
        }

        o.add(std::move(key), std::move(val));
      }

      return true;
    }
  };

  template<>
  struct Serializer<String> {
    template<typename Context>
    static SerializedObject write(String const & o, Context const &) {
      return SerializedObject::MakeText(o);
    }

    template<typename Context>
    static bool read(SerializedObject const & s, String & o, Context const &) {
      switch (s.getType()) {
      case SerializedObject::Type_Float: mem::construct(&o, toString(s.asFloat())); break;
      case SerializedObject::Type_Int: mem::construct(&o, toString(s.asInt())); break;
      case SerializedObject::Type_Text: mem::construct(&o, s.asText()); break;
      default: return false;
      }
      return true;
    }
  };

  template<>
  struct Serializer<StringView> {
    template<typename Context>
    static SerializedObject write(StringView const & o, Context const &) {
      return SerializedObject::MakeText(o);
    }
  };

  template<>
  struct Serializer<Filename> {
    template<typename Context>
    static SerializedObject write(Filename const & o, Context const &) {
      return SerializedObject::MakeText(o.path());
    }

    template<typename Context>
    static bool read(SerializedObject const & s, Filename & o, Context const &) {
      String text;
      if (!s.read(text)) {
        return false;
      }
      mem::construct(&o, text);
      return true;
    }
  };

  template<>
  struct Serializer<URI> {
    template<typename Context>
    static SerializedObject write(URI const & o, Context const &) {
      return SerializedObject::MakeText(o.str());
    }

    template<typename Context>
    static bool read(SerializedObject const & s, URI & o, Context const &) {
      String text;
      if (!s.read(text)) {
        return false;
      }
      mem::construct(&o, text);
      return true;
    }
  };

  template<>
  struct Serializer<UUID> {
    template<typename Context>
    static SerializedObject write(UUID const & o, Context const &) {
      return SerializedObject::MakeText(o.toString());
    }

    template<typename Context>
    static bool read(SerializedObject const & s, UUID & o, Context const &) {
      if (!s.isText()) {
        return false;
      }

      mem::construct(&o, s.asText());
      return true;
    }
  };
} // namespace bfc

