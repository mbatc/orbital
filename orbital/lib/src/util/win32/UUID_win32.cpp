#include "util/UUID.h"
#include "core/Stream.h"

#include <rpc.h>

namespace bfc {
  static UUID s_null;

  UUID UUID::New() {
    UUID uuid;
    UuidCreate((GUID*)uuid.m_data);
    return uuid;
  }

  UUID::UUID() {
    memset(m_data, 0, size());
  }

  UUID::UUID(String const & str) {
    UuidFromStringA((RPC_CSTR)str.c_str(), (GUID*)m_data);
  }

  UUID::UUID(uint8_t bytes[Length]) {
    memcpy(m_data, bytes, size());
  }

  String UUID::toString() const {
    RPC_CSTR cstr;
    UuidToStringA((GUID *)m_data, &cstr);

    String ret = (char*)cstr;
    RpcStringFree(&cstr);
    return ret;
  }

  uint8_t const * UUID::begin() const {
    return m_data;
  }

  uint8_t const * UUID::end() const {
    return m_data + size();
  }

  uint8_t const * UUID::data() const {
    return m_data;
  }

  bool UUID::operator==(UUID const & rhs) const {
    return memcmp(m_data, rhs.m_data, size()) == 0;
  }

  bool UUID::operator!=(UUID const & rhs) const {
    return !(*this == rhs);
  }

  bool UUID::isNull() const {
    return *this == s_null;
  }

  uint64_t hash(UUID const& uuid) {
    return hash(uuid.m_data);
  }

  int64_t write(Stream * pStream, UUID const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!pStream->write(pValue[i].data(), pValue[i].size())) {
        return i;
      }
    }
    return count;
  }

  int64_t read(Stream * pStream, UUID * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!pStream->read(pValue[i].data(), pValue[i].size())) {
        return i;
      }
    }
    return count;
  }
} // namespace bfc
