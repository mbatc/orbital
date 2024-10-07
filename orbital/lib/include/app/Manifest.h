#pragma once

#include "../core/Serialize.h"
#include "../util/VersionNumber.h"

namespace bfc {
  namespace app {
    struct BFC_API Manifest {
      Vector<String> plugins;  ///< Dynamically loaded plugins. Added to the .module file post build.
      Vector<String> required; ///< Plugins linked in the applications .module file
    };
  } // namespace app

  template<>
  struct BFC_API Serializer<app::Manifest> {
    static SerializedObject write(app::Manifest const & o);
    static bool read(SerializedObject const & s, app::Manifest & o);
  };
}
