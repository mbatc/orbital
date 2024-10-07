#include "core/Serialize.h"
#include "core/Stream.h"
#include "util/YAML.h"

namespace bfc {
  bool serialize(URI const & uri, SerializedObject const & o, DataFormat const & fmt) {
    switch (fmt) {
    case DataFormat_Binary: return writeURI<SerializedObject>(uri, o);
    case DataFormat_YAML: return writeYAML(uri, o);
    }
    return false;
  }

  std::optional<SerializedObject> deserialize(URI const & uri, DataFormat const & fmt) {
    switch (fmt) {
    case DataFormat_Binary: return readURI<SerializedObject>(uri);
    case DataFormat_YAML: return readYAML(uri);
    }
    return {};
  }
} // namespace bfc
