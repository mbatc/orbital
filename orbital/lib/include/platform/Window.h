#pragma once

#include "../core/Core.h"
#include "../core/String.h"
#include "../core/Timestamp.h"
#include "../core/Filename.h"
#include "../math/MathTypes.h"
#include "../platform/Events.h"
#include "../platform/KeyCode.h"

namespace bfc {
  namespace platform {
    enum WindowStyle {
      WindowStyle_None          = 0,
      WindowStyle_NoDecoration  = 1 << 0,
      WindowStyle_NoTaskBarIcon = 1 << 1,
      WindowStyle_TopMost       = 1 << 2,
    };

    class BFC_API Window
    {
      struct Impl;
    public:
      Window(Events *pEvents);
      Window(Window &&o) = delete;
      Window(Window const &o) = delete;
      ~Window();

      void show();
      void hide();

      void setTitle(String const & title);
      void setSize(Vec2i const & size);
      void setPosition(Vec2i const & position);
      void setDragDrop(bool enabled);

      String getTitle() const;
      Vec2i getSize() const;
      Vec2i getPosition() const;
      Vec2i clientToScreen(Vec2i const & clientPosition) const;
      Vec2i screenToClient(Vec2i const & clientPosition) const;

      void setAlpha(float alpha);
      float getDpiScale() const;

      void setFocus();
      bool isFocussed() const;
      bool isMinimized() const;
      bool isMaximized() const;

      Events* getEvents() const;
      void* getPlatformHandle() const;

      WindowStyle getStyle() const;
      void setStyle(WindowStyle const& style);

      static Window* getForegroundWindow();
      static Window* findWindow(Vec2i const& screenPosition);

    private:
      WindowStyle m_style = WindowStyle_None;

      Events *m_pEvents = nullptr;
      Impl *m_pData = nullptr;
    };

    enum MessageBoxButton {
      MessageBoxButton_Unknown = -1,
      MessageBoxButton_Abort,
      MessageBoxButton_Retry,
      MessageBoxButton_Ignore,
      MessageBoxButton_Count,
    };

    BFC_API MessageBoxButton errorMessageBox(char const* title, char const* message);
  }

  namespace events
  {
    // Window events
    struct ResizeWindow {
      platform::Window * pWindow = nullptr;
      Vec2i size = Vec2i(0);
    };

    struct MoveWindow {
      platform::Window * pWindow = nullptr;
      Vec2i              position = Vec2i(0);
    };

    struct ActivateWindow {
      platform::Window * pWindow = nullptr;
    };

    struct DeactivateWindow {
      platform::Window * pWindow = nullptr;
    };

    struct CreateWindow {
      platform::Window * pWindow = nullptr;
    };
    
    struct DestroyWindow {
      platform::Window * pWindow = nullptr;
    };
    
    struct CloseWindow {
      platform::Window * pWindow = nullptr;
    };

    struct PaintWindow {
      platform::Window * pWindow = nullptr;
    };

    struct WindowDPIChanged {
      platform::Window * pWindow = nullptr;

      Vec2  dpi;
      Vec2i size     = Vec2i(0);
      Vec2i position = Vec2i(0);
    };

    struct KeyDown {
      platform::Window * pWindow = nullptr;
      KeyCode code = KeyCode_Unknown;
      Timestamp ts;
    };

    struct KeyUp {
      platform::Window * pWindow = nullptr;
      KeyCode code = KeyCode_Unknown;
      Timestamp ts;
    };

    struct MouseDown {
      platform::Window * pWindow = nullptr;
      MouseButton code = MouseButton_Unknown;
      Timestamp   ts;
    };

    struct MouseUp {
      platform::Window * pWindow = nullptr;
      MouseButton code = MouseButton_Unknown;
      Timestamp ts;
    };

    struct Character {
      platform::Window * pWindow = nullptr;
      char c = 0;
      Timestamp ts;
    };

    struct MouseScroll {
      platform::Window * pWindow    = nullptr;
      float amount = 0;
      bool horizontal = false;
      Timestamp ts;
    };

    struct DroppedFiles {
      platform::Window * pWindow = nullptr;
      Vector<Filename>   files;
    };

    struct MousePosition {
      platform::Window * pWindow  = nullptr;
      Vec2 position = Vec2(0);
      Timestamp ts;
    };
  }
}