#pragma once

#include "input/InputDevice.h"

namespace bfc {
  InputDevice::InputDevice(int64_t buttonCount, int64_t axisCount) {
    m_axes.resize(axisCount);
    m_buttons.resize(buttonCount);
  }

  void InputDevice::update(Timestamp ts) {
    for (Button& bt : m_buttons) {
      if (bt.callback) {
        bt.callback(&bt);
      } else {
        bt.isPressed     = false;
        bt.isReleased    = false;
        bt.lastTimestamp = ts;
      }
    }

    for (Axis& axis : m_axes) {
      if (axis.callback) {
        axis.callback(&axis);
      } else {
        axis.change    = 0;
        axis.lastValue = axis.value;
      }
    }

    for (ButtonEvent const & ev : m_buttonEvents) {
      Button &bt       = m_buttons[ev.index];
      bt.isPressed    |= !bt.isDown && ev.isDown;
      bt.isReleased   |= bt.isDown && !ev.isDown;
      bt.isDown        = ev.isDown;
      bt.lastTimestamp = ts;

      if (bt.isPressed || bt.isReleased) {
        bt.stateChangeTS = ts;
      }
    }

    for (AxisEvent const & ev : m_axisEvents) {
      Axis & axis = m_axes[ev.index];
      axis.value  = ev.value;
    }

    for (Axis & axis : m_axes) {
      axis.change = axis.value - axis.lastValue;
    }

    m_axisEvents.clear();
    m_buttonEvents.clear();
  }

  void InputDevice::customAxisUpdate(int64_t index, std::function<void(Axis *)> callback) {
    m_axes[index].callback = callback;
  }

  void InputDevice::customButtonUpdate(int64_t index, std::function<void(Button *)> callback) {
    m_buttons[index].callback = callback;
  }

  int64_t InputDevice::getButtonCount() const {
    return m_buttons.size();
  }

  int64_t InputDevice::getAxisCount() const {
    return m_axes.size();
  }

  float InputDevice::getAxis(int64_t index) const {
    return m_axes[index].value;
  }

  float InputDevice::getAxisPrev(int64_t index) const {
    return m_axes[index].lastValue;
  }

  float InputDevice::getAxisChange(int64_t index) const {
    return m_axes[index].change;
  }

  bool InputDevice::getButton(int64_t index) const {
    return m_buttons[index].isDown;
  }

  bool InputDevice::getPressed(int64_t index) const {
    return m_buttons[index].isPressed;
  }

  bool InputDevice::getReleased(int64_t index) const {
    return m_buttons[index].isReleased;
  }

  Timestamp InputDevice::getTimeUp(int64_t index) const {
    return m_buttons[index].isDown ? 0 : m_buttons[index].lastTimestamp.length - m_buttons[index].stateChangeTS.length;
  }

  Timestamp InputDevice::getTimeDown(int64_t index) const {
    return m_buttons[index].isDown ? m_buttons[index].lastTimestamp.length - m_buttons[index].stateChangeTS.length : 0;
  }

  void InputDevice::updateButton(int64_t index, bool isDown, Timestamp ts) {
    ButtonEvent ev;
    ev.index  = index;
    ev.isDown = isDown;
    ev.ts     = ts;
    m_buttonEvents.pushBack(ev);
  }

  void InputDevice::updateAxis(int64_t index, float val) {
    AxisEvent ev;
    ev.index = index;
    ev.value = val;
    m_axisEvents.pushBack(ev);
  }
}
