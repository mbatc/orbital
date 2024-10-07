#include "app/Manifest.h"

namespace bfc {
  SerializedObject Serializer<app::Manifest>::write(app::Manifest const& o) {
    return SerializedObject::MakeMap({
      { "plugins", serialize(o.plugins) },
      { "required", serialize(o.required) }
    });
  }

  bool Serializer<app::Manifest>::read(SerializedObject const & s, app::Manifest & o) {
    s.get("plugins").readOrConstruct(o.plugins);
    s.get("required").readOrConstruct(o.required);
    return true;
  }
}