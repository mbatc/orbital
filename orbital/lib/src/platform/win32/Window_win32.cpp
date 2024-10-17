#include "platform/Window.h"

#ifdef BFC_WINDOWS

#include "platform/Display.h"

#include <windows.h>
#include <windowsx.h>

namespace bfc {
  namespace platform {
    Map<void *, Window *> g_handleToWindow;

    static void GetWin32Style(WindowStyle style, DWORD * pWinStyle, DWORD * pWinStyleEx) {
      *pWinStyle   = 0;
      *pWinStyleEx = 0;
      if (style & WindowStyle_NoDecoration)
        *pWinStyle = WS_POPUP;
      else
        *pWinStyle = WS_OVERLAPPEDWINDOW;

      if (style & WindowStyle_NoTaskBarIcon)
        *pWinStyleEx = WS_EX_TOOLWINDOW;
      else
        *pWinStyleEx = WS_EX_APPWINDOW;

      if (style & WindowStyle_TopMost)
        *pWinStyleEx |= WS_EX_TOPMOST;
    }

    static LRESULT WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static KeyCode GetKeyCode(WPARAM wParam);

    struct Window::Impl {
      HWND hWnd    = nullptr;
      ATOM clsAtom = 0;
    };

    Window::Window(Events* pEvents) {
      m_pData   = new Impl;
      m_pEvents = pEvents;

      HINSTANCE hInstance = GetModuleHandle(0);

      // Register Window
      static ATOM clsAtom = 0;
      if (clsAtom == 0) {
        WNDCLASSEX cls    = {0};
        cls.cbSize        = sizeof(WNDCLASSEX);
        cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        cls.hInstance     = hInstance;
        cls.hIcon         = 0;
        cls.hIconSm       = 0;
        cls.hCursor       = NULL;
        cls.lpszClassName = "bfcWindow";
        cls.style         = 0;
        cls.lpfnWndProc   = WindowProc;
        clsAtom           = RegisterClassEx(&cls);
      }

      // Create Window
      m_pData->hWnd = CreateWindowEx(0, MAKEINTATOM(clsAtom), "Window", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, 0, 0, hInstance, 0);

      setDragDrop(true);

      UpdateWindow(m_pData->hWnd);

      g_handleToWindow.add(m_pData->hWnd, this);
    }

    Window::~Window() {
      g_handleToWindow.erase(m_pData->hWnd);

      DestroyWindow(m_pData->hWnd);
      // UnregisterClass(MAKEINTATOM(m_pData->clsAtom), GetModuleHandle(0));
      delete m_pData;
    }

    void Window::setTitle(String const & title) {
      SetWindowText(m_pData->hWnd, title.c_str());
    }

    void Window::setSize(Vec2i const & size) {
      DWORD winStyle, winStyleEx;
      RECT  rect;
      rect.left   = 0;
      rect.right  = size.x;
      rect.top    = 0;
      rect.bottom = size.y;
      GetWin32Style(m_style, &winStyle, &winStyleEx);
      AdjustWindowRectEx(&rect, winStyle, false, winStyleEx);
      SetWindowPos(m_pData->hWnd, 0, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOMOVE | SWP_NOREPOSITION);
    }

    void Window::setPosition(Vec2i const & pos) {
      DWORD winStyle, winStyleEx;
      RECT  rect;
      rect.left   = pos.x;
      rect.right  = 0;
      rect.top    = pos.y;
      rect.bottom = 0;
      GetWin32Style(m_style, &winStyle, &winStyleEx);
      AdjustWindowRectEx(&rect, winStyle, false, winStyleEx);
      SetWindowPos(m_pData->hWnd, 0, rect.left, rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOREPOSITION);
    }

    void Window::setDragDrop(bool enabled) {
      DragAcceptFiles(m_pData->hWnd, enabled);
    }

    bfc::String Window::getTitle() const {
      int          len = GetWindowTextLength(m_pData->hWnd);
      Vector<char> buffer(len + 1, 0);
      GetWindowText(m_pData->hWnd, buffer.data(), len);
      return buffer.data();
    }

    Vec2i Window::getSize() const {
      RECT rect;
      GetClientRect(m_pData->hWnd, &rect);
      return Vec2i(rect.right, rect.bottom);
    }

    Vec2i Window::getPosition() const {
      return clientToScreen({0, 0});
    }

    Vec2i Window::clientToScreen(Vec2i const & clientPosition) const {
      POINT pos;
      pos.x = clientPosition.x;
      pos.y = clientPosition.y;
      ::ClientToScreen(m_pData->hWnd, &pos);
      return {pos.x, pos.y};
    }

    Vec2i Window::screenToClient(Vec2i const & screenPosition) const {
      POINT pos;
      pos.x = screenPosition.x;
      pos.y = screenPosition.y;
      ::ScreenToClient(m_pData->hWnd, &pos);
      return {pos.x, pos.y};
    }

    void Window::setAlpha(float alpha) {
      if (alpha < 1.0f) {
        DWORD style = ::GetWindowLongW(m_pData->hWnd, GWL_EXSTYLE) | WS_EX_LAYERED;
        ::SetWindowLongW(m_pData->hWnd, GWL_EXSTYLE, style);
        ::SetLayeredWindowAttributes(m_pData->hWnd, 0, (BYTE)(255 * alpha), LWA_ALPHA);
      } else {
        DWORD style = ::GetWindowLongW(m_pData->hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED;
        ::SetWindowLongW(m_pData->hWnd, GWL_EXSTYLE, style);
      }
    }

    float Window::getDpiScale() const {
      return findDisplay(*this).dpiScale;
    }

    void Window::setFocus() {
      ::SetFocus(m_pData->hWnd);
    }

    bool Window::isFocussed() const {
      return ::GetFocus() == m_pData->hWnd;
    }

    bool Window::isMinimized() const {
      return ::IsMinimized(m_pData->hWnd);
    }

    bool Window::isMaximized() const {
      return ::IsMaximized(m_pData->hWnd);
    }

    Events* Window::getEvents() const {
      return m_pEvents;
    }

    void * Window::getPlatformHandle() const {
      return m_pData->hWnd;
    }

    void Window::show() {
      ShowWindow(m_pData->hWnd, SW_SHOW);
    }

    void Window::hide() {
      ShowWindow(m_pData->hWnd, SW_HIDE);
    }

    WindowStyle Window::getStyle() const {
      return m_style;
    }

    void Window::setStyle(WindowStyle const & style) {
      m_style = style;
      DWORD winStyle, winStyleEx;
      GetWin32Style(style, &winStyle, &winStyleEx);

      Vec2i pos  = getPosition();
      Vec2i size = getSize();

      // Apply flags and position (since it is affected by flags)
      ::SetWindowLong(m_pData->hWnd, GWL_STYLE, winStyle);
      ::SetWindowLong(m_pData->hWnd, GWL_EXSTYLE, winStyleEx);

      RECT rect;
      rect.left   = pos.x;
      rect.top    = pos.y;
      rect.right  = pos.x + size.x;
      rect.bottom = pos.y + size.y;

      ::AdjustWindowRectEx(&rect, winStyle, FALSE, winStyleEx); // Client to Screen
      ::SetWindowPos(m_pData->hWnd, (style & WindowStyle_TopMost) ? HWND_TOPMOST : HWND_NOTOPMOST, pos.x, pos.y, size.x, size.y,
                     SWP_NOACTIVATE | SWP_FRAMECHANGED);
      ::ShowWindow(m_pData->hWnd, SW_SHOWNA); // This is necessary when we alter the style
    }

    Window * Window::getForegroundWindow() {
      HWND      hWnd     = GetForegroundWindow();
      Window ** ppWindow = g_handleToWindow.tryGet(hWnd);
      return ppWindow ? *ppWindow : 0;
    }

    Window * Window::findWindow(Vec2i const & screenPosition) {
      HWND      hWnd     = WindowFromPoint({screenPosition.x, screenPosition.y});
      Window ** ppWindow = g_handleToWindow.tryGet(hWnd);
      return ppWindow ? *ppWindow : 0;
    }

#undef CreateWindow

    LRESULT WindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
      Timestamp ts = Timestamp::now();

      Window ** ppWindow = g_handleToWindow.tryGet((void *)hWnd);
      if (ppWindow != nullptr) {
        Window & window = **ppWindow;
        Events* pEvents = window.getEvents();
        switch (Msg) {
        case WM_CLOSE: pEvents->broadcast(events::CloseWindow{&window}); break;
        case WM_DESTROY: pEvents->broadcast(events::DestroyWindow{&window}); break;
        case WM_CREATE: pEvents->broadcast(events::CreateWindow{&window}); break;
        case WM_PAINT: pEvents->broadcast(events::PaintWindow{&window}); break;
        // case WM_DPICHANGED: {
        //   LPRECT pRect    = (LPRECT)lParam;
        //   Vec2i  position = Vec2i((float)pRect->left, (float)pRect->top);
        //   Vec2i  size     = Vec2i(float(pRect->right - pRect->left), float(pRect->bottom - pRect->top));
        //   pEvents->broadcast(events::WindowDPIChanged{&window, Vec2(LOWORD(wParam), HIWORD(wParam)), size, position});
        //
        //   ::SetWindowPos(hWnd, 0, position.x, position.y, size.x, size.y, SWP_NOZORDER | SWP_NOREPOSITION);
        // } break;
        case WM_ACTIVATE:
          if (wParam == WA_INACTIVE)
            pEvents->broadcast(events::DeactivateWindow{&window});
          else
            pEvents->broadcast(events::ActivateWindow{&window});
          break;
        case WM_SIZE: {
          Vec2i sz = {LOWORD(lParam), HIWORD(lParam)};
          pEvents->broadcast(events::ResizeWindow{&window, sz});
        } break;
        case WM_MOVE: {
          Vec2i pos = {(int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)};
          pEvents->broadcast(events::MoveWindow{&window, pos});
        } break;
        case WM_KEYDOWN: pEvents->broadcast(events::KeyDown{&window, GetKeyCode(wParam), ts}); break;
        case WM_KEYUP: pEvents->broadcast(events::KeyUp{&window, GetKeyCode(wParam), ts}); break;
        case WM_CHAR: pEvents->broadcast(events::Character{&window, (char)wParam, ts}); break;
        case WM_MOUSEMOVE: pEvents->broadcast(events::MousePosition{&window, Vec2(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)), ts}); break;
        case WM_LBUTTONUP: pEvents->broadcast(events::MouseUp{&window, MouseButton_Left, ts}); break;
        case WM_LBUTTONDOWN: pEvents->broadcast(events::MouseDown{&window, MouseButton_Left, ts}); break;
        case WM_RBUTTONUP: pEvents->broadcast(events::MouseUp{&window, MouseButton_Right, ts}); break;
        case WM_RBUTTONDOWN: pEvents->broadcast(events::MouseDown{&window, MouseButton_Right, ts}); break;
        case WM_MBUTTONUP: pEvents->broadcast(events::MouseUp{&window, MouseButton_Middle, ts}); break;
        case WM_MBUTTONDOWN: pEvents->broadcast(events::MouseDown{&window, MouseButton_Middle, ts}); break;
        case WM_MOUSEWHEEL: pEvents->broadcast(events::MouseScroll{&window, float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA, false, ts}); break;
        case WM_MOUSEHWHEEL: pEvents->broadcast(events::MouseScroll{&window, float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA, true, ts}); break;
        case WM_DROPFILES: {
          events::DroppedFiles e{&window};

          HDROP hDrop = (HDROP)wParam;
          UINT numDropped = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
          for (UINT i = 0; i < numDropped; ++i) {
            UINT len = DragQueryFile(hDrop, i, NULL, 0);
            bfc::Vector<char> buffer;
            buffer.resize(len + 1, 0);

            if (DragQueryFileA(hDrop, i, buffer.data(), (UINT)buffer.size()) != 0)
              e.files.pushBack(buffer.data());
          }

          pEvents->broadcast(e);
          break;
        }
        }
      }

      return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    KeyCode GetKeyCode(WPARAM wParam) {
      switch (wParam) {
      case 'A': return KeyCode_A;
      case 'B': return KeyCode_B;
      case 'C': return KeyCode_C;
      case 'D': return KeyCode_D;
      case 'E': return KeyCode_E;
      case 'F': return KeyCode_F;
      case 'G': return KeyCode_G;
      case 'H': return KeyCode_H;
      case 'I': return KeyCode_I;
      case 'J': return KeyCode_J;
      case 'K': return KeyCode_K;
      case 'L': return KeyCode_L;
      case 'M': return KeyCode_M;
      case 'N': return KeyCode_N;
      case 'O': return KeyCode_O;
      case 'P': return KeyCode_P;
      case 'Q': return KeyCode_Q;
      case 'R': return KeyCode_R;
      case 'S': return KeyCode_S;
      case 'T': return KeyCode_T;
      case 'U': return KeyCode_U;
      case 'V': return KeyCode_V;
      case 'W': return KeyCode_W;
      case 'X': return KeyCode_X;
      case 'Y': return KeyCode_Y;
      case 'Z': return KeyCode_Z;
      case '0': return KeyCode_0;
      case '1': return KeyCode_1;
      case '2': return KeyCode_2;
      case '3': return KeyCode_3;
      case '4': return KeyCode_4;
      case '5': return KeyCode_5;
      case '6': return KeyCode_6;
      case '7': return KeyCode_7;
      case '8': return KeyCode_8;
      case '9': return KeyCode_9;
      case VK_LEFT: return KeyCode_LeftArrow;
      case VK_RIGHT: return KeyCode_RightArrow;
      case VK_UP: return KeyCode_UpArrow;
      case VK_DOWN: return KeyCode_DownArrow;
      case VK_RETURN: return KeyCode_Enter;
      case VK_BACK: return KeyCode_Backspace;
      case VK_SPACE: return KeyCode_Space;
      case VK_OEM_COMMA: return KeyCode_Comma;
      case VK_OEM_PERIOD: return KeyCode_Period;
      // return KeyCode_Apostrophe;
      // return KeyCode_SemiColon;
      // return KeyCode_Grave;
      case VK_TAB: return KeyCode_Tab;
      case VK_CAPITAL: return KeyCode_CapsLock;
      case VK_RSHIFT:
      case VK_LSHIFT:
      case VK_SHIFT: return KeyCode_Shift;
      case VK_LCONTROL:
      case VK_RCONTROL:
      case VK_CONTROL: return KeyCode_Control;
      case VK_MENU: return KeyCode_Alt;
      // return KeyCode_Menu;
      // return KeyCode_Slash;
      // return KeyCode_Backslash;
      // return KeyCode_OpenBracket;
      // return KeyCode_CloseBracket;
      case VK_OEM_MINUS: return KeyCode_Minus;
      case VK_OEM_PLUS: return KeyCode_Equals;
      case VK_INSERT: return KeyCode_Insert;
      case VK_DELETE: return KeyCode_Delete;
      case VK_HOME: return KeyCode_Home;
      case VK_END: return KeyCode_End;
      case VK_PRIOR: return KeyCode_PageUp;
      case VK_NEXT: return KeyCode_PageDown;
      case VK_OEM_7: return KeyCode_Apostrophe;
      case VK_OEM_5: return KeyCode_Pipe;
      }

      return KeyCode_Unknown;
    }

    MessageBoxButton errorMessageBox(char const * title, char const * message) {
      switch (MessageBoxA(nullptr, message, title, MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION | MB_SYSTEMMODAL)) {
      case IDABORT: return MessageBoxButton_Abort;
      case IDRETRY: return MessageBoxButton_Retry;
      case IDIGNORE: return MessageBoxButton_Ignore;
      }
      return MessageBoxButton_Unknown;
    }
  } // namespace platform
} // namespace bfc

#endif
