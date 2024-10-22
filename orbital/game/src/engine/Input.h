#pragma once

#include "core/String.h"
#include "core/Map.h"
#include "math/MathTypes.h"
#include "input/InputDevice.h"
#include "Subsystem.h"

namespace engine {
  namespace events {
    struct OnAnalogInput {
      
    };

    struct OnDigitalInput {
      
    };
  }

  struct InputDescriptor {
    bfc::String inputDevice;
    int64_t     inputIndex;
    float       multiplier = 1.0f;
  };

  class Input : public Subsystem {
  public:
    class Channel {
    public:
      void update(Input *pInputs);

      float value() const;
      float previous() const;
      float change() const;

      bool down(float threshold = 1.0f) const;
      bool pressed(float threshold = 1.0f) const;
      bool released(float threshold = 1.0f) const;

      Input::Channel & mapButton(InputDescriptor const & desc);
      Input::Channel & mapAnalog(InputDescriptor const & desc);

      Input::Channel & setRange(bfc::Vec2 const & range = {-1, 1});
      bfc::Vec2 range() const;

    private:
      bfc::Vector<InputDescriptor> m_mappedButtons;
      bfc::Vector<InputDescriptor> m_mappedAnalogs;

      float m_value     = 0;
      float m_previous  = 0;
      bfc::Vec2 m_range = { -1, 1 };
    };

    Input();

    /// Get the names of all available input devices.
    bfc::Vector<bfc::String> getInputDevices() const;
    /// Map an input device instance to an input name.
    bool setInputDevice(bfc::StringView const & name, bfc::InputDevice * pDevice);
    /// Get the input device instance by name.
    bfc::InputDevice * getInputDevice(bfc::StringView const & name);

    Channel & channel(bfc::StringView const & name);

    bool mapAnalogInput(bfc::StringView const & name, InputDescriptor const & desc);
    bool mapButtonInput(bfc::StringView const & name, InputDescriptor const & desc);

    float value(bfc::StringView const & name) const;
    float previous(bfc::StringView const & name) const;
    float change(bfc::StringView const & name) const;

    bool down(bfc::StringView const & name, float threshold = 1.0f) const;
    bool pressed(bfc::StringView const & name, float threshold = 1.0f) const;
    bool released(bfc::StringView const & name, float threshold = 1.0f) const;

    virtual void loop(Application * pApp);

  private:
    bfc::Map<bfc::String, Channel> m_channels;
    bfc::Map<bfc::String, bfc::InputDevice *> m_devices;
  };
}
