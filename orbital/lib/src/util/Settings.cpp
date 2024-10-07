#include "util/Settings.h"
#include "util/Log.h"

#include "core/Map.h"
#include "core/Timestamp.h"
#include "core/URI.h"
#include "platform/Events.h"

#include <filesystem>

namespace bfc {
  void Settings::attach(Events *pEvents) {
    m_events.add(pEvents);
  }

  void Settings::detach(Events *pEvents) {
    m_events.erase(pEvents);
  }

  void Settings::add(StringView const & path, SerializedObject const & defaultValue) {
    BFC_LOG_INFO("core", "Setting added: %.*s", path.length(), path.data());

    m_defaults.addOrSet(path, defaultValue);
    m_values.tryAdd(path, defaultValue);
  }

  void Settings::add(StringView const & path, bool defaultValue) {
    add(path, SerializedObject(defaultValue));
  }

  void Settings::add(StringView const & path, int64_t defaultValue) {
    add(path, SerializedObject(defaultValue));
  }

  void Settings::add(StringView const & path, double defaultValue) {
    add(path, SerializedObject(defaultValue));
  }

  void Settings::add(StringView const & path, StringView const & defaultValue) {
    add(path, SerializedObject(defaultValue));
  }

  void Settings::setObject(StringView const & path, SerializedObject const & value) {
    SerializedObject & val = m_values.getOrAdd(path);

    bool changed = val != value;
    val          = value;

    if (changed) {
      m_changed     = true;
      m_lastChanged = Timestamp::now();

      events::SettingUpdated updateEvent;
      updateEvent.lastValue = val;
      updateEvent.newValue  = value;
      updateEvent.path      = path;

      for (Events *pEvents : m_events)
        pEvents->broadcast(updateEvent);
    }
  }

  void Settings::set(StringView const & path, bool value) {
    setObject(path, SerializedObject(value));
  }

  void Settings::set(StringView const & path, int64_t value) {
    setObject(path, SerializedObject(value));
  }

  void Settings::set(StringView const & path, double value) {
    setObject(path, SerializedObject(value));
  }

  void Settings::set(StringView const & path, StringView const & value) {
    setObject(path, SerializedObject(value));
  }

  SerializedObject Settings::getObject(StringView const & path, SerializedObject const & defaultValue) {
    return m_values.getOr(path, defaultValue);
  }

  bool Settings::getBool(StringView const & path, bool defaultValue) {
    bool value = defaultValue;
    if (SerializedObject * pSetting = m_values.tryGet(path)) {
      pSetting->read(value);
    }
    return value;
  }

  int64_t Settings::getInt(StringView const & path, int64_t defaultValue) {
    int64_t value = defaultValue;
    if (SerializedObject * pSetting = m_values.tryGet(path)) {
      pSetting->read(value);
    }
    return value;
  }

  double Settings::getFloat(StringView const & path, double defaultValue) {
    double value = defaultValue;
    if (SerializedObject * pSetting = m_values.tryGet(path)) {
      pSetting->read(value);
    }
    return value;
  }

  String Settings::getString(StringView const & path, StringView const & defaultValue) {
    String value = defaultValue;
    if (SerializedObject * pSetting = m_values.tryGet(path)) {
      pSetting->read(value);
    }
    return value;
  }

  bool Settings::exists(StringView const & path) {
    return m_values.contains(path);
  }

  bool Settings::reset(StringView const & path, bool asGroup) {
    BFC_LOG_INFO("core", "Reset settings (group: %s): %.*s", asGroup ? "true" : "false", path.length(), path.data());

    bool success = true;
    if (asGroup) {
      for (String const& subPath : enumerate(path)) {
        success &= reset(subPath, false);
      }
    } else {
      SerializedObject * pDefault = m_defaults.tryGet(path);
      if (pDefault != nullptr) {
        setObject(path, *pDefault);
        success = true;
      }
    }
    return success;
  }

  bool Settings::clear(StringView const & path, bool asGroup) {
    BFC_LOG_INFO("core", "Clear settings (group: %s): %.*s", asGroup ? "true" : "false", path.length(), path.data());

    bool success = true;
    if (asGroup) {
      for (String const & subPath : enumerate(path)) {
        success &= clear(subPath, false);
      }
    } else {
      success &= m_values.erase(path);
      success &= m_defaults.erase(path);
    }
    return success;
  }

  bool Settings::forEach(std::function<bool(StringView, SerializedObject)> const & callback, StringView const & root) {
    const bool exportAll = root.length() == 0;
    for (auto & [name, value] : m_values) {
      if (exportAll || name.startsWith(root)) {
        if (!callback(name, value)) {
          return false;
        }
      }
    }
    return true;
  }

  Vector<String> Settings::enumerate(StringView const & root) {
    Vector<String> ret;
    forEach(
      [&](StringView const & path, SerializedObject const & value) {
        ret.pushBack(path);
        return true;
      },
      root);

    return ret;
  }

  SerializedObject Settings::serialize(StringView const & root) {
    SerializedObject ret;
    forEach(
      [&](StringView const & path, SerializedObject const & value) {
        ret.add(path, value);
        return true;
      },
      root);

    return ret;
  }

  bool Settings::deserialize(SerializedObject const & values) {
    if (!values.isMap()) {
      return false;
    }

    for (auto & [name, value] : values.asMap()) {
      m_values.addOrSet(name, value);
    }

    return true;
  }

  bool Settings::save(URI const & uri, StringView const & root, DataFormat fmt) {
    return bfc::serialize(uri, serialize(root), fmt);
  }

  bool Settings::load(URI const & uri, DataFormat fmt) {
    auto serializedSettings = bfc::deserialize(uri, fmt);
    if (!serializedSettings) {
      return false;
    }

    deserialize(serializedSettings.value());
    return true;
  }
}
