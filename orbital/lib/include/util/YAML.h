#pragma once

#include "core/Core.h"
#include "core/Serialize.h"

namespace bfc {
  class URI;
  class SerializedObject;

  BFC_API std::optional<SerializedObject> readYAML(URI const & uri);
  BFC_API std::optional<SerializedObject> readYAML(Stream * pStream);

  BFC_API bool writeYAML(URI const & uri, SerializedObject const & object);
  BFC_API bool writeYAML(Stream * pStream, SerializedObject const & object);

  template<typename T>
  bool writeYAML(URI const & uri, T const & o) {
    return writeYAML(uri, serialize(o));
  }

  template<typename T>
  std::optional<T> readYAML(URI const & uri) {
    std::optional<SerializedObject> serialized = readYAML(uri, &serialized);
    if (!serialized)
      return {};

    Uninitialized<T> buffer;
    if (!serialized.read(buffer.get()))
      return {};

    return std::move(buffer.get());
  }
}
