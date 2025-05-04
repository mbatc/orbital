#pragma once

#include "Core.h"
#include "StringView.h"

namespace bfc {
  namespace detail {
    template<class T>
    struct TypeName {
    public:
      static StringView name() {
        static char       funcName[] = BFC_FUNCTION;
        static StringView typeName   = []() {
          StringView ret   = funcName;
          int64_t    begin = ret.find("<") + 1;
          int64_t    end   = ret.findLast(">");
          return ret.substr(begin, end - begin);
        }();

        return typeName;
      }

      static char const * c_str() {
        static char const * ptr = []() {
          StringView  _name    = name();
          static char buffer[] = BFC_FUNCTION;
          std::memcpy(buffer, _name.data(), _name.length());
          buffer[_name.length()] = 0;
          return buffer;
        }();
        return ptr;
      }

      static constexpr uint64_t hash_code() {
        return bfc::hash(name());
      }
    };
  } // namespace detail

  /**
   * @brief Portable unique runtime type information.
   *
   * The purpose of this class is to provided basic runtime type information that is
   * consistent across DLL boundaries and compilers.
   */
  class BFC_API type_index {
    template<typename Type>
    friend type_index TypeID();

    template<typename T>
    constexpr type_index(std::in_place_type_t<T>)
      : m_name(detail::TypeName<T>::c_str())
      , m_hashCode(detail::TypeName<T>::hash_code()) {}

  public:
    inline char const * name() const {
      return m_name;
    }

    inline uint64_t hash_code() const {
      return m_hashCode;
    }

    inline bool operator==(type_index const & rhs) const {
      return m_hashCode == rhs.m_hashCode;
    }

    inline bool operator!=(type_index const & rhs) const {
      return m_hashCode != rhs.m_hashCode;
    }

  private:
    // TODO: Perhaps incorpoorate size into the ID to reduce possible ID collisions
    char const * m_name     = "";
    uint64_t     m_hashCode = 0;
  };

  template<typename T>
  type_index TypeID() {
    static type_index info(std::in_place_type_t<std::decay_t<T>>{});
    return info;
  }

  template<typename T>
  type_index const & TypeID(const T &) {
    return TypeID<T>();
  }
} // namespace Fractal

namespace std {
  template<>
  struct hash<bfc::type_index> {
    size_t operator()(bfc::type_index const & o) const {
      return o.hash_code();
    }
  };
} // namespace std
