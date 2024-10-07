#pragma once

#include "Core.h"
#include "StringView.h"

namespace bfc {
  namespace detail {
    template<typename T>
    struct template_name {
      static StringView name() {
        static char       funcName[]   = BFC_FUNCTION;
        static StringView templateName = [=]() {
          StringView  func        = funcName;
          int64_t     start       = func.find('<');
          int64_t     end         = func.findLast('>');
          func = func.substr(start + 1, end - start);
          int64_t templateEnd     = func.find('<');
          return templateEnd == npos ? "" : func.substr(0, templateEnd);
        }();

        return templateName;
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

      static uint64_t hash_code() {
        static uint64_t code = []() {
          StringView _name = name();
          return _name == "" ? 0 : bfc::hash(name());
        }();

        return code;
      }
    };
  } // namespace detail

  class BFC_API template_info {
    template<typename T>
    friend template_info const & templateid();

    template_info(char const * name, uint64_t hashCode)
      : m_name(name)
      , m_hashCode(hashCode) {}

  public:
    template_info(template_info && o)      = delete;
    template_info(template_info const & o) = delete;

    inline char const * name() const {
      return m_name;
    }

    inline uint64_t hash_code() const {
      return m_hashCode;
    }

    inline bool operator==(template_info const & rhs) const {
      return m_hashCode == rhs.m_hashCode;
    }

    inline bool operator!=(template_info const & rhs) const {
      return m_hashCode != rhs.m_hashCode;
    }

  private:
    char const * m_name     = "";
    uint64_t     m_hashCode = 0;
  };

  class BFC_API template_index {
  public:
    inline template_index(template_info const & o)
      : m_pInfo(&o) {}

    inline char const * name() const {
      return m_pInfo->name();
    }

    inline uint64_t hash_code() const {
      return m_pInfo->hash_code();
    }

    inline bool operator==(template_index const & rhs) const {
      return *m_pInfo == *rhs.m_pInfo;
    }

    inline bool operator==(template_info const & rhs) const {
      return *m_pInfo == rhs;
    }

    inline bool operator!=(template_index const & rhs) const {
      return *m_pInfo != *rhs.m_pInfo;
    }

    inline bool operator!=(template_info const & rhs) const {
      return *m_pInfo != rhs;
    }

  private:
    template_info const * m_pInfo = nullptr;
  };

  template<typename T>
  template_info const & templateid() {
    static template_info info(detail::template_name<T>::c_str(), detail::template_name<T>::hash_code());
    return info;
  }

  inline uint64_t hash(bfc::template_index const & info) {
    return info.hash_code();
  }

  inline uint64_t hash(bfc::template_info const & info) {
    return info.hash_code();
  }
} // namespace bfc
