#pragma once

#include "core/URI.h"

namespace engine {
  namespace builtin_assets {
    namespace mesh {
      inline static const bfc::URI plane  = bfc::URI::File("engine:models/primitives/plane.obj");
      inline static const bfc::URI cube   = bfc::URI::File("engine:models/primitives/cube.obj");
      inline static const bfc::URI sphere = bfc::URI::File("engine:models/primitives/sphere.obj");
    }
  }
}