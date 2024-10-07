#pragma once

#include "../core/Core.h"
#include "../core/String.h"

namespace bfc {
  class BFC_API UUID {
  public:
    static inline constexpr int64_t Length = 16ll;

    /// Generate a new UUID.
    static UUID New();

    /// Default construct a UUID.
    /// This will be an empty UUID, filled with null bytes.
    UUID();

    /// Create a UUID from its string representation.
    UUID(String const & str);

    /// Create a UUID from an array of bytes
    UUID(uint8_t bytes[Length]);

    /// Convert this UUID to a string.
    String toString() const;

    uint8_t const * begin() const;
    uint8_t const * end() const;
    uint8_t const * data() const;

    bool operator==(UUID const & rhs) const;
    bool operator!=(UUID const & rhs) const;

    static inline constexpr int64_t size() {
      return Length;
    }

    bool isNull() const;

    BFC_API friend uint64_t hash(UUID const & uuid);

  private:
    union {
      struct {
        uint8_t timeLow[4];
        uint8_t timeMid[2];
        uint8_t timeHighAndVersion[2];
        uint8_t clockSeqAndRes[2];
        uint8_t node[6];
      };

      uint8_t m_data[Length];
    };
  };

  BFC_API int64_t write(Stream * pStream, UUID const * pValue, int64_t count);
  BFC_API int64_t read(Stream * pStream, UUID * pValue, int64_t count);
} // namespace bfc
