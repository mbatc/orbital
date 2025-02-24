#include "input/Mouse.h"

#ifdef BFC_WINDOWS

#include <windows.h>

namespace bfc {
  BFC_API bfc::Vec2i getCursorPosition() {
    POINT p;
    ::GetCursorPos(&p);
    return { p.x, p.y };
  }

  BFC_API void setCursorPosition(bfc::Vec2i position) {
    ::SetCursorPos(position.x, position.y);
  }

  static HICON sysIcons[CursorIcon_Count] = {
    LoadCursor(NULL, IDC_ARROW),    // CursorIcon_Arrow = 0,
    LoadCursor(NULL, IDC_IBEAM),    // CursorIcon_TextInput,
    LoadCursor(NULL, IDC_SIZEALL),  // CursorIcon_ResizeAll,
    LoadCursor(NULL, IDC_SIZENS),   // CursorIcon_ResizeNS,
    LoadCursor(NULL, IDC_SIZEWE),   // CursorIcon_ResizeEW,
    LoadCursor(NULL, IDC_SIZENESW), // CursorIcon_ResizeNESW,
    LoadCursor(NULL, IDC_SIZENWSE), // CursorIcon_ResizeNWSE,
    LoadCursor(NULL, IDC_HAND),     // CursorIcon_Hand,
    LoadCursor(NULL, IDC_CROSS)     // CursorIcon_NotAllowed,
  };

  BFC_API void setCursorIcon(CursorIcon icon) {
    SetCursor(sysIcons[icon]);
  }

  BFC_API CursorIcon getCursorIcon() {
    HICON hIcon = GetCursor();
    for (int64_t i = 0; i < CursorIcon_Count; ++i) {
      if (sysIcons[i] == hIcon) {
        return (CursorIcon)i;
      }
    }
    return CursorIcon_None;
  }

  BFC_API void setCursorVisible(bool visible) {
    ::ShowCursor(visible);
  }
}

#endif