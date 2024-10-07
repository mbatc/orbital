#pragma once

#include "../core/Vector.h"
#include "../math/MathTypes.h"

namespace bfc {
  namespace platform {
    class Window;
  }

  struct BFC_API Display {
    Vec2i mainPos = Vec2i(0);
    Vec2i mainSize = Vec2i(0);
    Vec2i workPos = Vec2i(0);
    Vec2i workSize = Vec2i(0);
    float dpiScale = 1;
    bool isPrimary = false;
  };

  BFC_API Display findDisplay(platform::Window const &window);

  /// Enumerate physical displays.
  BFC_API Vector<Display> enumerateDisplays();
}
