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
  enum DataFormat {
    DataFormat_YAML,
    DataFormat_Binary,
    DataFormat_Count,
  };

  BFC_API bool serialize(URI const & uri, SerializedObject const & o, DataFormat const & fmt = DataFormat_YAML);

  BFC_API std::optional<SerializedObject> deserialize(URI const & uri, DataFormat const & fmt = DataFormat_YAML);

  template<typename T, typename Serializer>
  SerializedObject serialize(Serializer & ar, T const & o) {
    return ar(o);
  }

  template<typename T, typename Serializer>
  std::optional<T> deserialize(Serializer & ar, SerializedObject const & serialized) {
    Uninitialized<T> buf;
    if (!ar(serialized, buf)) {
      return std::nullopt;
    }
    return buf.take();
  }

  template<typename T, typename Serializer>
  bool read(Serializer & ar, SerializedObject const & serialized, T & o) {
    Uninitialized<T> buf;
    if (!ar(serialized, buf)) {
      return false;
    }
    o = buf.take();
    return true;
  }

  template<typename T, typename Serializer>
  bool serialize(Serializer & ar, URI const & uri, T const & o, DataFormat const & fmt = DataFormat_YAML) {
    return serialize(uri, serialize<Serializer, <T>(ar, o), fmt);
  }

  template<typename T, typename Serializer>
  std::optional<T> deserialize(Serializer & ar, URI const & uri, DataFormat const & fmt = DataFormat_YAML) {
    Uninitialized<T>                buf;
    std::optional<SerializedObject> serialized = deserialize(uri, fmt);
    if (!serialized) {
      return {};
    }
    if (!ar(serialized.value(), buf)) {
      return {};
    }

    return buf.take();
  }

  namespace detail {
    struct NoSerialization {};
  }
  template<typename T>
  struct Serialization{};

  namespace detail {
    struct DummySerializer {};
    template<typename T, typename U = void>
    struct has_serialize_write : std::false_type {};
    template<typename T>
    struct has_serialize_write<T, std::void_t<decltype(bfc::Serialization<T>::write(DummySerializer{}, *(T *)0))>>
      : std::true_type {};

    template<typename T, typename U = void>
    struct has_serialize_write_concrete : std::false_type {};
    template<typename T>
    struct has_serialize_write_concrete<T, std::void_t<decltype(bfc::Serialization<T>::write)>> : std::true_type {};

    template<typename T, typename U = void>
    struct has_serialize_read : std::false_type {};
    template<typename T>
    struct has_serialize_read<
      T, std::void_t<decltype(bfc::Serialization<T>::read(DummySerializer{}, bfc::SerializedObject{}, *(T *)0))>>
      : std::true_type {};

    template<typename T, typename U = void>
    struct has_serialize_read_concrete : std::false_type {};
    template<typename T>
    struct has_serialize_read_concrete<T, std::void_t<decltype(bfc::Serialization<T>::read)>> : std::true_type {};

    template<typename T, typename U = void>
    struct has_serialize_intrusive_read : std::false_type {};
    template<typename T>
    struct has_serialize_intrusive_read<
      T, std::void_t<decltype(T::Meta::read(DummySerializer{}, bfc::SerializedObject{}, *(T *)0))>>
      : std::true_type {};

    template<typename T, typename U = void>
    struct has_serialize_intrusive_read_concrete : std::false_type {};
    template<typename T>
    struct has_serialize_intrusive_read_concrete<T, std::void_t<decltype(T::Meta::read)>> : std::true_type {};

    template<typename T, typename U = void>
    struct has_serialize_intrusive_write : std::false_type {};
    template<typename T>
    struct has_serialize_intrusive_write<T, std::void_t<decltype(T::Meta::write(DummySerializer{}, *(T *)0))>>
      : std::true_type {};

    template<typename T, typename U = void>
    struct has_serialize_intrusive_write_concrete : std::false_type {};
    template<typename T>
    struct has_serialize_intrusive_write_concrete<T, std::void_t<decltype(T::Meta::write)>> : std::true_type {};
  } // namespace detail

  template<typename T>
  inline static constexpr bool has_serialize_write =
    detail::has_serialize_write<T>::value || detail::has_serialize_write_concrete<T>::value;

  template<typename T>
  inline static constexpr bool has_serialize_read =
    detail::has_serialize_read<T>::value || detail::has_serialize_read_concrete<T>::value;

  template<typename T>
  inline static constexpr bool has_serialize_intrusive_write =
    detail::has_serialize_intrusive_write<T>::value || detail::has_serialize_intrusive_write_concrete<T>::value;

  template<typename T>
  inline static constexpr bool has_serialize_intrusive_read =
    detail::has_serialize_intrusive_read<T>::value || detail::has_serialize_intrusive_read_concrete<T>::value;

  template<typename Impl>
  class SerializerBase {
  private:
    /// Read from reflection
    template<typename T, int64_t I, typename... Members>
    static bool writeMember(Impl & ar, SerializedObject & dst, T const & o,
                            Reflection<T, Members...> const & reflected) {
      if constexpr (reflected.isMember<I>()) {
        dst.get(reflected.name<I>()) = ar(reflected.get<I>(&o));
        return true;
      } else {
        return false;
      }
    }

    template<typename T, typename... Members, int64_t... Indices>
    static SerializedObject writeMembers(Impl & ar, T const & o,
                                         Reflection<T, Members...> const & reflection,
                                         std::integer_sequence<int64_t, Indices...>) {
      SerializedObject ret;
      (writeMember<T, Indices>(ar, ret, o, reflection), ...);
      return ret;
    }

    template<typename T, int64_t I, typename... Members>
    static bool readMember(Impl & ar, SerializedObject const & src, T & o,
                           Reflection<T, Members...> const & reflected) {
      if constexpr (reflected.isMember<I>()) {
        ar(src.get(reflected.name<I>()), reflected.get<I>(&o));
        return true;
      } else {
        return false;
      }
    }

    template<typename T, typename... Members, int64_t... Indices>
    static void readMembers(Impl & ar, SerializedObject const & src, T & o,
                            Reflection<T, Members...> const & reflection, std::integer_sequence<int64_t, Indices...>) {
      (readMember<T, Indices>(ar, src, o, reflection), ...);
    }

  public:
    Impl & AsImpl() {
      return *(Impl *)this;
    }

    /// Serialize `o`.
    template<typename T>
    SerializedObject operator()(T const & o) {
      if constexpr (bfc::has_serialize_write<T>) {
        return Serialization<T>::write(AsImpl(), o);
      } else if constexpr (bfc::has_serialize_intrusive_write<T>) {
        return T::Meta::write(AsImpl(), o);
      } else if constexpr (bfc::has_reflect_v<T>) {
        auto reflection = reflect<T>();
        return writeMembers(AsImpl(), o, reflection, std::make_integer_sequence<int64_t, reflection.size()>{});
      } else if constexpr (std::is_floating_point_v<T>) {
        return SerializedObject::MakeFloat((double)o);
      } else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
        if constexpr (BFC_HAS_MEMBER(EnumValueMap<T>, mapping)) {
          auto & mapping = EnumValueMap<T>::mapping;
          using MappingT = decltype(EnumValueMap<T>::mapping);

          if constexpr (is_vector_v<MappingT>) {
            // Serialize mapped values
            if (o < 0 || o >= mapping.size()) {
              return SerializedObject::Empty();
            } else {
              return serialize(AsImpl(), mapping[o]);
            }
          } else if constexpr (is_map_v<MappingT>) {
            static_assert(std::is_same_v<MappingT::key_type, T>, "EnumValueMap<T>::mapping keys must be of type 'T'");
            auto * pValue = mapping.tryGet(o);
            if (pValue == nullptr) {
              return SerializedObject::Empty();
            } else {
              return serialize(AsImpl(), *pValue);
            }
          } else {
            static_assert(false, "EnumValueMap<T>::mapping is not a supported type. Must be Vector<any> or Map<T, any>");
          }
        } else {
          // Just serialize as an integer
          return SerializedObject::MakeInt((int64_t)o);
        }
      } else {
        static_assert(false, "No serialization writer implemented for type `T`");
        return false;
      }
    }

    template<typename T>
    bool operator()(SerializedObject const & s, T const & o) {
      if constexpr (bfc::has_serialize_read<T>) {
        return Serialization<T>::read(AsImpl(), s, o);
      } else if constexpr (bfc::has_serialize_intrusive_read<T>) {
        return T::Serialization::read(AsImpl(), s, o);
      } else {
        static_assert(false, "Deserializing with const ref is usually for tag types (e.g. ReadOrConstruct). Did you do something strange?");
        return false;
      }
    }

    /// Deserialize into `o`
    template<typename T>
    bool operator()(SerializedObject const & s, T & o) {
      if constexpr (bfc::has_serialize_read<T>) {
        return Serialization<T>::read(AsImpl(), s, o);
      } else if constexpr (bfc::has_serialize_intrusive_read<T>) {
        return T::Meta::read(AsImpl(), s, o);
      } else if constexpr (bfc::has_reflect_v<T>) {
        memset(&o, 0, sizeof(T));
        auto reflection = reflect<T>();
        readMembers(AsImpl(), s, o, reflection, std::make_integer_sequence<int64_t, reflection.size()>{});
        return true;
      } else if constexpr (std::is_floating_point_v<T>) {
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
      } else if constexpr (std::is_integral_v<T> || std::is_enum_v<T>) {
        if constexpr (BFC_HAS_MEMBER(EnumValueMap<T>, mapping)) {
          auto & mapping = EnumValueMap<T>::mapping;
          using MappingT = decltype(EnumValueMap<T>::mapping);

          if constexpr (is_vector_v<MappingT>) {
            // Read value and find mapping in Vector
            using value_type = MappingT::value_type;
            Uninitialized<value_type> buffer;
            if (!AsImpl()(s, buffer)) {
              return false;
            }

            bool success = false;
            for (int64_t i = 0; i < mapping.size(); ++i) {
              if (buffer.get() == mapping[i]) {
                o       = (T)i;
                success = true;
                break;
              }
            }
            buffer.destruct();
            return success;
          } else if constexpr (is_map_v<MappingT>) {
            static_assert(std::is_same_v<MappingT::key_type, T>, "EnumValueMap<T>::mapping keys must be of type 'T'");
            // Read value and find mapping in Map
            using value_type = MappingT::mapped_type;
            Uninitialized<value_type> buffer;
            if (!AsImpl()(s, buffer)) {
              return false;
            }

            bool success = false;
            for (auto & [key, value] : mapping) {
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
      } else {
        static_assert(
          false,
          "No serialization reader implemented for type `T`");
        return false;
      }
    }
  };

  class DefaultSerializer : public SerializerBase<DefaultSerializer> {};

  template<typename T>
  SerializedObject serialize(T const & o) {
    DefaultSerializer ar;
    return serialize(ar, o);
  }

  template<typename T>
  std::optional<T> deserialize(SerializedObject const & serialized) {
    DefaultSerializer ar;
    return deserialize(ar, o);
  }

  template<typename T>
  bool read(SerializedObject const & serialized, T & o) {
    DefaultSerializer ar;
    return read(ar, serialized, o);
  }

  template<typename T>
  bool serialize(URI const & uri, T const & o, DataFormat const & fmt = DataFormat_YAML) {
    DefaultSerializer ar;
    return serialize(ar, uri, o, fmt);
  }

  template<typename T>
  std::optional<T> deserialize(URI const & uri, DataFormat const & fmt = DataFormat_YAML) {
    DefaultSerializer ar;
    return deserialize<T>(ar, uri, fmt);
  }

  /// EnumValueMap can be specialized to map enums of type 'T' to an arbitrary value of any type
  template<typename T>
  struct EnumValueMap {
    // inline static Vector<any> const mapping;
    // inline static Map<T, any> const mapping;
  };
  BFC_DEFINE_MEMBER_CHECK(mapping);

  template<typename T, typename... Args>
  struct ReadOrConstruct {
    ReadOrConstruct(T & value, Args&&... args)
      : value(value)
      , args(std::tuple{std::forward<Args>(args)...}) {}

    T & value;
    mutable std::tuple<Args...> args;
  };

  template<typename T, typename... Args>
  ReadOrConstruct(T & value, Args&&... args) -> ReadOrConstruct<T, Args...>;

  template<typename T, typename... Args>
  struct Serialization<ReadOrConstruct<T, Args...>> {
    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, ReadOrConstruct<T, Args...> const & o) {
      if (!ar(s, o.value)) {
        bfc::invoke(&mem::construct<T, Args...>, std::tuple_cat(std::tuple{ &o.value }, std::move(o.args)));
      }
      return true;
    }
  };

  template<typename T>
  struct Serialization<Uninitialized<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Uninitialized<T> const & o) {
      return ar(o.get());
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Uninitialized<T> & o) {
      return ar(s, o.get());
    }
  };

  template<typename T>
  struct Serialization<std::optional<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, std::optional<T> const & o) {
      if (o.has_value()) {
        return ar(o.value());
      } else {
        return SerializedObject::Empty();
      }
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, std::optional<T> & o) {
      if (s.isEmpty()) {
        mem::construct(&o, std::nullopt);
        return true;
      }

      Uninitialized<T> value;
      if (!ar(s, value)) {
        return false;
      }

      o.emplace(value.take());
      return true;
    }
  };

  template<typename T>
  struct Serialization<Vector2<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Vector2<T> const & o) {
      return SerializedObject::MakeArray({ar(o.x), ar(o.y)});
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Vector2<T> & o) {
      bool success = true;
      for (int i = 0; i < 2; ++i) {
        success &= ar(s.at(i), o[i]);
      }
      return success;
    }
  };

  template<typename T>
  struct Serialization<Vector3<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Vector3<T> const & o) {
      return SerializedObject::MakeArray({ ar(o.x), ar(o.y), ar(o.z) });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Vector3<T> & o) {
      bool success = true;
      for (int i = 0; i < 3; ++i) {
        success &= ar(s.at(i), o[i]);
      }
      return success;
    }
  };

  template<typename T>
  struct Serialization<Vector4<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Vector4<T> const & o) {
      return SerializedObject::MakeArray({ ar(o.x), ar(o.y), ar(o.z), ar(o.w) });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Vector4<T> & o) {
      bool success = true;
      for (int i = 0; i < 4; ++i) {
        success &= ar(s.at(i), o[i]);
      }
      return success;
    }
  };

  template<typename T>
  struct Serialization<Quaternion<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Quaternion<T> const & o) {
      return SerializedObject::MakeArray({ar(o.x), ar(o.y), ar(o.z), ar(o.w)});
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Quaternion<T> & o) {
      bool success = true;
      for (int i = 0; i < 4; ++i) {
        success &= ar(s.at(i), o[i]);
      }
      return success;
    }
  };

  template<typename T>
  struct Serialization<Matrix<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Matrix<T> const & o) {
      return SerializedObject::MakeArray({
        ar(o[0]),
        ar(o[1]),
        ar(o[2]),
        ar(o[3])
      });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Matrix<T> & o) {
      bool success = true;
      for (int i = 0; i < 4; ++i) {
        success &= ar(s.at(i), o[i]);
      }
      return success;
    }
  };

  template<typename T>
  struct Serialization<Vector<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Vector<T> const & o) {
      return ar(o.getView());
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Vector<T> & o) {
      mem::construct(&o);

      if (!s.isArray()) {
        return s.isEmpty();
      }

      o.reserve(s.size());
      for (SerializedObject const & elm : s.asArray()) {
        Uninitialized<T> val;
        if (!ar(elm, val)) {
          return false;
        }
        o.pushBack(val.take());
      }

      return true;
    }
  };

  template<typename T>
  struct Serialization<Span<T>> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Span<T> const & o) {
      SerializedObject s = SerializedObject::MakeArray();
      s.asArray().resize(o.size());
      for (auto & [i, item] : enumerate(o)) {
        s.asArray()[i] = ar(item);
      }
      return s;
    }
  };

  template<typename Key, typename Value>
  struct Serialization<Pair<Key, Value>> {
    static inline String firstName  = "first";
    static inline String secondName = "second";

    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Pair<Key, Value> const & o) {
      return SerializedObject::MakeMap({
        { firstName, ar(o.first) },
        { secondName, ar(o.second) }
        });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Pair<Key, Value> & o) {
      bool success = ar(s.get(firstName), o.first);
      success     &= ar(s.get(secondName), o.second);
      return success;
    }
  };

  template<typename Key, typename Value>
  struct Serialization<Map<Key, Value>> {
    static inline String keyName = "key";
    static inline String valName = "value";

    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Map<Key, Value> const & o) {
      SerializedObject s = SerializedObject::MakeArray();
      for (auto const & [key, value] : o) {
        s.pushBack(SerializedObject::MakeMap({
          { keyName, ar(key) }, { valName, ar(value) }}));
      }
      return s;
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Map<Key, Value> & o) {
      if (s.isEmpty()) {
        mem::construct(&o);
        return true;
      }

      if (!s.isArray()) {
        return false;
      }

      mem::construct(&o);
      for (SerializedObject const & elm : s.asArray()) {
        if (!elm.isMap() && elm.size() != 2) {
          return false;
        }

        Uninitialized<Key>   key;
        Uninitialized<Value> val;
        if (!(ar(elm.get(keyName), key) && ar(elm.get(valName), val))) {
          return false;
        }

        o.add(key.take(), val.take());
      }

      return true;
    }
  };

  template<>
  struct Serialization<String> {
    template<typename Serializer>
    static SerializedObject write(Serializer &, String const & o) {
      return SerializedObject::MakeText(o);
    }

    template<typename Serializer>
    static bool read(Serializer &, SerializedObject const & s, String & o) {
      switch (s.getType()) {
      case SerializedObject::Type_Float: mem::construct(&o, toString(s.asFloat())); break;
      case SerializedObject::Type_Int:   mem::construct(&o, toString(s.asInt())); break;
      case SerializedObject::Type_Text:  mem::construct(&o, s.asText()); break;
      default: return false;
      }
      return true;
    }
  };

  // template<>
  // struct Serialization<String> {
  //   template<typename Serializer>
  //   static SerializedObject write(Serializer &, String const & o) {
  //     return SerializedObject::MakeText(o);
  //   }
  // 
  //   template<typename Serializer>
  //   static bool read(Serializer &, SerializedObject const & s, String & o) {
  //     switch (s.getType()) {
  //     case SerializedObject::Type_Text: mem::construct(&o, s.asText()); return true;
  //     case SerializedObject::Type_Float: mem::construct(&o, bfc::toString(s.asFloat())); return true;
  //     case SerializedObject::Type_Int: mem::construct(&o, bfc::toString(s.asInt())); return true;
  //     case SerializedObject::Type_Bool: mem::construct(&o, s.asBool() ? "true" : "false"); return true;
  //     default: return false;
  //     }
  //   }
  // };

  template<>
  struct Serialization<StringView> {
    template<typename Serializer>
    static SerializedObject write(Serializer & , StringView const & o) {
      return SerializedObject::MakeText(o);
    }
  };

  template<>
  struct Serialization<Filename> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, Filename const & o) {
      return ar(o.path());
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, Filename & o) {
      Uninitialized<String> text;
      if (!ar(text)) {
        return false;
      }
      mem::construct(&o, text.take());
      return true;
    }
  };

  template<>
  struct Serialization<URI> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, URI const & o) {
      return ar(o.str());
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, URI & o) {
      Uninitialized<String> text;
      if (!ar(s, text)) {
        return false;
      }
      mem::construct(&o, text.take());
      return true;
    }
  };

  template<>
  struct Serialization<UUID> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, UUID const & o) {
      return ar(o.toString());
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, UUID & o) {
      Uninitialized<String> text;
      if (!ar(s, text)) {
        return false;
      }
      mem::construct(&o, text.take());
      return true;
    }
  };
} // namespace bfc

