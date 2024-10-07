#pragma once

#include "InputDevice.h"

namespace bfc {
  class Events;
  class EventListener;
  
  enum KeyCode;

  class BFC_API Keyboard : public InputDevice {
  public:
    Keyboard();
    Keyboard(Events *pEvents);

    /// Attach this keyboard to an event system.
    virtual void attachTo(Events * pEvents) override;
    /// Get the time since the key was last down.
    Timestamp getTimeUp(KeyCode code) const;
    /// Get the time the key has been held.
    Timestamp getTimeDown(KeyCode code) const;

    /// Check if a key is held down.
    bool isDown(KeyCode code) const;
    /// Check if a key is pressed.
    bool isPressed(KeyCode code) const;
    /// Check if a key is released.
    bool isReleased(KeyCode code) const;

  private:
    std::shared_ptr<EventListener> m_pListener;
  };
}
