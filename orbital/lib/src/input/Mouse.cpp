#include "input/Mouse.h"
#include "platform/Window.h"
#include "platform/KeyCode.h"

namespace bfc {
  Mouse::Mouse()
    : InputDevice(MouseButton_ExtendedCount, MouseAxis_Count) {
    customAxisUpdate(MouseAxis_Scroll, [](InputDevice::Axis * pAxis) {
      pAxis->change    = pAxis->value - pAxis->lastValue;
      pAxis->value     = 0;
      pAxis->lastValue = pAxis->value;
    });

    customAxisUpdate(MouseAxis_ScrollX, [](InputDevice::Axis * pAxis) {
      pAxis->change    = pAxis->value - pAxis->lastValue;
      pAxis->value     = 0;
      pAxis->lastValue = pAxis->value;
    });
  }

  Mouse::Mouse(Events* pEvents)
      : Mouse() {
    attachTo(pEvents);
  }

  void Mouse::attachTo(Events* pEvents) {
    if (pEvents == nullptr) {
      m_pListener = nullptr;
      return;
    }

    m_pListener = pEvents->addListener();

    m_pListener->on<events::MouseDown>(
        [this](events::MouseDown const& e) {
          updateButton(e.code, true, e.ts);
          return false;
        });

    m_pListener->on<events::MouseUp>(
        [this](events::MouseUp const& e) {
          updateButton(e.code, false, e.ts);
          return false;
        });

    m_pListener->on<events::MousePosition>(
        [this](events::MousePosition const& e) {
          updateAxis(MouseAxis_X, e.position.x);
          updateAxis(MouseAxis_Y, e.position.y);
          return false;
        });

    m_pListener->on<events::MouseScroll>(
        [this](events::MouseScroll const& e) {
          if (e.horizontal) {
            updateAxis(MouseAxis_ScrollX, e.amount);
          }
          else {
            updateAxis(MouseAxis_Scroll, e.amount);
          }
          return false;
        });

    m_pListener->on<events::DeactivateWindow>([=](events::DeactivateWindow const& o) {
      Timestamp ts = Timestamp::now();
      for (int64_t i = 0; i < getButtonCount(); ++i)
        updateButton(i, false, ts);
      return false;
    });
  }

  Vec2 Mouse::getPosition() const {
    return Vec2(getAxis(MouseAxis_X), getAxis(MouseAxis_Y));
  }

  Vec2 Mouse::getLastPosition() const {
    return Vec2(getAxisPrev(MouseAxis_X), getAxisPrev(MouseAxis_Y));
  }

  Vec2 Mouse::getPositionChange() const {
    return Vec2(getAxisChange(MouseAxis_X), getAxisChange(MouseAxis_Y));
  }

  float Mouse::getScroll() const {
    return getAxis(MouseAxis_Scroll);
  }

  float Mouse::getLastScroll() const {
    return getAxisPrev(MouseAxis_Scroll);
  }

  float Mouse::getScrollChange() const {
    return getAxisChange(MouseAxis_Scroll);
  }

  float Mouse::getScrollX() const {
    return getAxis(MouseAxis_ScrollX);
  }

  float Mouse::getLastScrollX() const {
    return getAxisPrev(MouseAxis_ScrollX);
  }

  float Mouse::getScrollChangeX() const {
    return getAxisChange(MouseAxis_ScrollX);
  }

  bool Mouse::isDown(MouseButton button) const {
    return InputDevice::getButton(button);
  }

  bool Mouse::isReleased(MouseButton button) const {
    return InputDevice::getReleased(button);
  }

  bool Mouse::isPressed(MouseButton button) const {
    return InputDevice::getPressed(button);
  }

  Timestamp Mouse::getTimeDown(MouseButton button) const {
    return InputDevice::getTimeDown(button);
  }

  Timestamp Mouse::getTimeUp(MouseButton button) const {
    return InputDevice::getTimeUp(button);
  }
}