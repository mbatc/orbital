#include "Input.h"
#include "Input/Keyboard.h"
#include "Input/Mouse.h"
#include "platform/KeyCode.h"
#include "Application.h"

using namespace bfc;

namespace engine {
  Input::Input()
    : Subsystem(bfc::TypeID<Input>(), "Input") {}

  void Input::Channel::update(Input * pInputs) {
    float newValue = 0.0f;
    for (InputDescriptor const & desc : m_mappedAnalogs) {
      InputDevice * pDevice = pInputs->getInputDevice(desc.inputDevice);
      if (pDevice == nullptr)
        continue;

      if (desc.inputIndex < pDevice->getAxisCount()) {
        float val = pDevice->getAxis(desc.inputIndex);
        newValue += val * desc.multiplier;
      }
    }

    for (InputDescriptor const & desc : m_mappedButtons) {
      InputDevice * pDevice = pInputs->getInputDevice(desc.inputDevice);
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

  float Input::Channel::value() const {
    return m_value;
  }

  float Input::Channel::previous() const {
    return m_previous;
  }

  float Input::Channel::change() const {
    return m_value - m_previous;
  }

  bool Input::Channel::down(float threshold) const {
    return math::abs(value()) >= threshold;
  }

  bool Input::Channel::pressed(float threshold) const {
    return math::abs(value()) > math::abs(previous()) && math::abs(change()) >= threshold;
  }

  bool Input::Channel::released(float threshold) const {
    return math::abs(value()) < math::abs(previous()) && math::abs(change()) >= threshold;
  }

  Input::Channel& Input::Channel::mapButton(InputDescriptor const & desc) {
    m_mappedButtons.pushBack(desc);
    return *this;
  }

  Input::Channel& Input::Channel::mapAnalog(InputDescriptor const & desc) {
    m_mappedAnalogs.pushBack(desc);
    return *this;
  }

  Input::Channel& Input::Channel::setRange(bfc::Vec2 const & range) {
    m_range = range;
    return *this;
  }

  bfc::Vec2 Input::Channel::range() const {
    return m_range;
  }

  Vector<String> Input::getInputDevices() const {
    return m_devices.getKeys();
  }

  bool Input::setInputDevice(bfc::StringView const & name, bfc::InputDevice * pDevice) {
    m_devices.addOrSet(name, pDevice);
    return true;
  }

  InputDevice * Input::getInputDevice(bfc::StringView const & name) {
    return m_devices.getOr(name, nullptr);
  }

  Input::Channel & Input::channel(bfc::StringView const & name) {
    return m_channels.getOrAdd(name);
  }

  bool Input::mapAnalogInput(bfc::StringView const & name, InputDescriptor const & desc) {
    channel(name).mapAnalog(desc);
    return true;
  }

  bool Input::mapButtonInput(bfc::StringView const & name, InputDescriptor const & desc) {
    channel(name).mapButton(desc);
    return true;
  }

  float Input::value(bfc::StringView const & name) const {
    return m_channels.contains(name) ? m_channels[name].value() : 0.0f;
  }

  float Input::previous(bfc::StringView const & name) const {
    return m_channels.contains(name) ? m_channels[name].previous() : 0.0f;
  }

  float Input::change(bfc::StringView const & name) const {
    return m_channels.contains(name) ? m_channels[name].change() : 0.0f;
  }

  bool Input::down(bfc::StringView const & name, float threshold) const {
    return m_channels.contains(name) ? m_channels[name].down(threshold) : 0.0f;
  }

  bool Input::pressed(bfc::StringView const & name, float threshold) const {
    return m_channels.contains(name) ? m_channels[name].pressed(threshold) : 0.0f;
  }

  bool Input::released(bfc::StringView const & name, float threshold) const {
    return m_channels.contains(name) ? m_channels[name].released(threshold) : 0.0f;
  }

  void Input::loop(Application * pApp) {
    BFC_UNUSED(pApp);

    for (auto& [name, pDevice] : m_devices) {
      pDevice->update(pApp->getDeltaTime());
    }

    for (auto& [name, channel] : m_channels) {
      channel.update(this);
    }
  }
} // namespace engine
