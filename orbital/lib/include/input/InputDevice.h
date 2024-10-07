#pragma once

#include "../core/Timestamp.h"
#include "../core/Vector.h"

namespace bfc {
  class Events;

  class BFC_API InputDevice {
  public:
    struct Axis {
      float value     = 0;
      float lastValue = 0;
      float change    = 0;

      std::function<void(Axis *)> callback;
    };

    struct Button {
      bool      isDown        = false;
      bool      isReleased    = false;
      bool      isPressed     = false;
      Timestamp lastTimestamp = 0;
      Timestamp stateChangeTS = 0;

      std::function<void(Button *)> callback;
    };

    InputDevice(int64_t buttonCount, int64_t axisCount);

    /// Attach this keyboard to an event system.
    virtual void attachTo(Events* pEvents) = 0;
    /// Update the input device.
    virtual void update(Timestamp ts);

    void customAxisUpdate(int64_t index, std::function<void(Axis *)> callback);
    void customButtonUpdate(int64_t index, std::function<void(Button *)> callback);

    int64_t getAxisCount() const;
    int64_t getButtonCount() const;

    float getAxis(int64_t index) const;
    float getAxisPrev(int64_t index) const;
    float getAxisChange(int64_t index) const;

    bool getButton(int64_t index) const;
    bool getPressed(int64_t index) const;
    bool getReleased(int64_t index) const;

    Timestamp getTimeUp(int64_t index) const;
    Timestamp getTimeDown(int64_t index) const;

    void updateButton(int64_t index, bool isDown, Timestamp ts);
    void updateAxis(int64_t index, float val);

  private:
    Vector<Axis> m_axes;
    Vector<Button> m_buttons;
  };
}
