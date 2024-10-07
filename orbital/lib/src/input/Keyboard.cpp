#include "input/Keyboard.h"
#include "platform/Events.h"
#include "platform/KeyCode.h"
#include "platform/Window.h"

namespace bfc {
  Keyboard::Keyboard()
      : InputDevice(KeyCode_Count, 0) {
  }

  Keyboard::Keyboard(Events *pEvents)
      : Keyboard() {
    attachTo(pEvents);
  }

  void Keyboard::attachTo(Events * pEvents) {
    if (pEvents == nullptr) {
      m_pListener = nullptr;
      return;
    }

    m_pListener = pEvents->addListener();

    m_pListener->on<events::KeyUp>([=](events::KeyUp const & o) {
      updateButton(o.code, false, o.ts);
      return false;
    });

    m_pListener->on<events::KeyDown>([=](events::KeyDown const & o) {
      updateButton(o.code, true, o.ts);
      return false;
    });

    m_pListener->on<events::DeactivateWindow>([=](events::DeactivateWindow const & o) {
      Timestamp ts = Timestamp::now();
      for (int64_t i = 0; i < getButtonCount(); ++i)
        updateButton(i, false, ts);
      return false;
    });
  }

  Timestamp Keyboard::getTimeUp(KeyCode code) const {
    return InputDevice::getTimeUp(code);
  }

  Timestamp Keyboard::getTimeDown(KeyCode code) const {
    return InputDevice::getTimeDown(code);
  }

  bool Keyboard::isDown(KeyCode code) const {
    return InputDevice::getButton(code);
  }

  bool Keyboard::isPressed(KeyCode code) const {
    return InputDevice::getPressed(code);
  }

  bool Keyboard::isReleased(KeyCode code) const {
    return InputDevice::getReleased(code);
  }
}
