#include "platform/Events.h"

#ifdef BFC_WINDOWS

#include <windows.h>

namespace bfc {
  void Events::update() {
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

#endif
