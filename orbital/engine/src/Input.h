#pragma once

#include "core/String.h"
#include "core/Map.h"
#include "math/MathTypes.h"
#include "input/InputDevice.h"
#include "Subsystem.h"

namespace engine {
  using namespace bfc;

  struct InputDescriptor {
    String  inputDevice;
    int64_t inputIndex = 0;
    float   multiplier = 1.0f;
  };

  namespace inputs {
    class Channel {
    public:
      void update();

      float value() const;
      float previous() const;
      float change() const;

      bool down(float threshold = 1.0f) const;
      bool pressed(float threshold = 1.0f) const;
      bool released(float threshold = 1.0f) const;

      Channel& mapButton(InputDescriptor const & desc);
      Channel& mapAnalog(InputDescriptor const & desc);
      Channel& setRange(Vec2 const & range = {-1, 1});
      Vec2 range() const;

    private:
      Vector<InputDescriptor> m_mappedButtons;
      Vector<InputDescriptor> m_mappedAnalogs;

      float     m_value    = 0;
      float     m_previous = 0;
      Vec2 m_range    = {-1, 1};
    };

    void update(Timestamp const & dt);

    /// Get the names of all available input devices.
    Vector<String> getInputDevices();

    /// Map an input device instance to an input name.
    bool setInputDevice(StringView const & name, InputDevice * pDevice);

    /// Get the input device instance by name.
    InputDevice * getInputDevice(StringView const & name);

    Channel& channel(StringView const & name);

    bool mapAnalogInput(StringView const & name, InputDescriptor const & desc);
    bool mapButtonInput(StringView const & name, InputDescriptor const & desc);

    float value(StringView const & name);
    float previous(StringView const & name);
    float change(StringView const & name);

    bool down(StringView const & name, float threshold = 1.0f);
    bool pressed(StringView const & name, float threshold = 1.0f);
    bool released(StringView const & name, float threshold = 1.0f);
  }
}
