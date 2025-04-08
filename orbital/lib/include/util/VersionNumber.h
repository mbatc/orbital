#pragma once

#include "../core/Core.h"
#include "../core/StringView.h"
#include "../core/Serialize.h"

namespace bfc {
  struct BFC_API VersionNumber {
    inline constexpr VersionNumber(uint32_t major = 0, uint32_t minor = 0, uint32_t patch = 0)
      : major(major)
      , minor(minor)
      , patch(patch) {}

    inline VersionNumber(StringView const & str) {
      StringView cursor = str;

      major  = (uint32_t)Scan::readInt(&cursor);
      cursor = cursor.substr(1);
      minor  = (uint32_t)Scan::readInt(&cursor);
      cursor = cursor.substr(1);
      patch  = (uint32_t)Scan::readInt(&cursor);
    }

    uint32_t major = 0;
    uint32_t minor = 0;
    uint32_t patch = 0;

    inline constexpr bool operator==(VersionNumber const & rhs) const {
      return major == rhs.major && minor == rhs.minor && patch == rhs.patch;
    }

    inline constexpr bool operator!=(VersionNumber const & rhs) const {
      return !operator==(rhs);
    }

    inline constexpr bool operator<(VersionNumber const & rhs) const {
      return major < rhs.major || (major == rhs.major && minor < rhs.minor || (minor == rhs.minor && patch < rhs.patch));
    }

    inline constexpr bool operator>(VersionNumber const & rhs) const {
      return major > rhs.major || (major == rhs.major && minor > rhs.minor || (minor == rhs.minor && patch > rhs.patch));
    }

    inline constexpr bool operator<=(VersionNumber const & rhs) const {
      return !operator>(rhs);
    }

    inline constexpr bool operator>=(VersionNumber const & rhs) const {
      return !operator<(rhs);
    }
  };

  inline String toString(VersionNumber const & value) {
    return String::format("%u.%u.%u", value.major, value.minor, value.patch);
  }

  template<>
  struct Serializer<VersionNumber> {
    template<typename Context>
    static SerializedObject write(VersionNumber const & o, Context const &) {
      return SerializedObject::MakeText(toString(o));
    }

    template<typename Context>
    static bool read(SerializedObject const & s, VersionNumber & o, Context const &) {
      if (!s.isText())
        return false;

      mem::construct(&o, s.asText());
      return true;
    }
  };
}
