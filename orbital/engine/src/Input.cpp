#include "Input.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include "platform/KeyCode.h"
#include "Application.h"

using namespace engine;
using namespace engine::inputs;

namespace {
  Map<String, Channel>       g_channels;
  Map<String, InputDevice *> g_devices;
}

namespace engine {
  namespace inputs {
    void update(Timestamp const & dt) {
      for (auto & [name, pDevice] : g_devices) {
        pDevice->update(dt);
      }

      for (auto & [name, channel] : g_channels) {
        channel.update();
      }
    }

    void Channel::update() {
      float newValue = 0.0f;
      for (InputDescriptor const & desc : m_mappedAnalogs) {
        InputDevice * pDevice = getInputDevice(desc.inputDevice);
        if (pDevice == nullptr)
          continue;

        if (desc.inputIndex < pDevice->getAxisCount()) {
          float val = pDevice->getAxis(desc.inputIndex);
          newValue += val * desc.multiplier;
        }
      }

      for (InputDescriptor const & desc : m_mappedButtons) {
        InputDevice * pDevice = getInputDevice(desc.inputDevice);
        if (pDevice == nullptr)
          continue;

        if (desc.inputIndex < pDevice->getButtonCount()) {
          bool state = pDevice->getButton(desc.inputIndex);
          newValue += (state ? 1 : 0) * desc.multiplier;
        }
      }

      m_previous = m_value;
      m_value    = math::clamp(newValue, m_range.x, m_range.y);
    }

    float Channel::value() const {
      return m_value;
    }

    float Channel::previous() const {
      return m_previous;
    }

    float Channel::change() const {
      return m_value - m_previous;
    }

    bool Channel::down(float threshold) const {
      return math::abs(value()) >= threshold;
    }

    bool Channel::pressed(float threshold) const {
      return math::abs(value()) > math::abs(previous()) && math::abs(change()) >= threshold;
    }

    bool Channel::released(float threshold) const {
      return math::abs(value()) < math::abs(previous()) && math::abs(change()) >= threshold;
    }

    Channel & Channel::mapButton(InputDescriptor const & desc) {
      m_mappedButtons.pushBack(desc);
      return *this;
    }

    Channel & Channel::mapAnalog(InputDescriptor const & desc) {
      m_mappedAnalogs.pushBack(desc);
      return *this;
    }

    Channel & Channel::setRange(Vec2 const & range) {
      m_range = range;
      return *this;
    }

    Vec2 Channel::range() const {
      return m_range;
    }

    Vector<String> getInputDevices() {
      return g_devices.getKeys();
    }

    bool setInputDevice(StringView const & name, InputDevice * pDevice) {
      g_devices.addOrSet(name, pDevice);
      return true;
    }

    InputDevice * getInputDevice(StringView const & name) {
      return g_devices.getOr(name, nullptr);
    }

    Channel & channel(StringView const & name) {
      return g_channels.getOrAdd(name);
    }

    bool mapAnalogInput(StringView const & name, InputDescriptor const & desc) {
      channel(name).mapAnalog(desc);
      return true;
    }

    bool mapButtonInput(StringView const & name, InputDescriptor const & desc) {
      channel(name).mapButton(desc);
      return true;
    }

    float value(StringView const & name) {
      return g_channels.contains(name) ? g_channels[name].value() : 0.0f;
    }

    float previous(StringView const & name) {
      return g_channels.contains(name) ? g_channels[name].previous() : 0.0f;
    }

    float change(StringView const & name) {
      return g_channels.contains(name) ? g_channels[name].change() : 0.0f;
    }

    bool down(StringView const & name, float threshold) {
      return g_channels.contains(name) ? g_channels[name].down(threshold) : 0.0f;
    }

    bool pressed(StringView const & name, float threshold) {
      return g_channels.contains(name) ? g_channels[name].pressed(threshold) : 0.0f;
    }

    bool released(StringView const & name, float threshold) {
      return g_channels.contains(name) ? g_channels[name].released(threshold) : 0.0f;
    }
  } // namespace inputs
} // namespace engine
