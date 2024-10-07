#pragma once

#include "InputDevice.h"
#include "../math/MathTypes.h"

namespace bfc {
  class Events;
  class EventListener;

  enum MouseButton;

  enum MouseAxis {
    MouseAxis_X,
    MouseAxis_Y,
    MouseAxis_Scroll,
    MouseAxis_ScrollX,
    MouseAxis_Count,
  };

  enum CursorIcon {
    CursorIcon_None = -1,
    CursorIcon_Arrow = 0,
    CursorIcon_TextInput,
    CursorIcon_ResizeAll,
    CursorIcon_ResizeNS,
    CursorIcon_ResizeEW,
    CursorIcon_ResizeNESW,
    CursorIcon_ResizeNWSE,
    CursorIcon_Hand,
    CursorIcon_NotAllowed,
    CursorIcon_Count,
  };

  class BFC_API Mouse : public InputDevice {
  public:
    Mouse();
    Mouse(Events* pEvents);

    void attachTo(Events* pEvents) override;

    Vec2 getPosition() const;
    Vec2 getLastPosition() const;
    Vec2 getPositionChange() const;

    float getScroll() const;
    float getLastScroll() const;
    float getScrollChange() const;

    float getScrollX() const;
    float getLastScrollX() const;
    float getScrollChangeX() const;

    bool isDown(MouseButton button) const;
    bool isReleased(MouseButton button) const;
    bool isPressed(MouseButton button) const;

    Timestamp getTimeDown(MouseButton button) const;
    Timestamp getTimeUp(MouseButton button) const;

  private:
    std::shared_ptr<EventListener> m_pListener;
  };

  BFC_API bfc::Vec2i getCursorPosition();

  BFC_API void setCursorPosition(bfc::Vec2i position);

  BFC_API void setCursorIcon(CursorIcon icon);

  BFC_API void setCursorVisible(bool visible);
}
