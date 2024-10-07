#pragma once

#include "../core/String.h"
#include "../core/Serialize.h"
#include "../core/SerializedObject.h"
#include "../core/Timestamp.h"
#include "../core/Set.h"

namespace bfc {
  namespace events {
    struct SettingUpdated {
      StringView       path;      ///< Path to the setting updated.
      SerializedObject lastValue; ///< Previous value of the setting.
      SerializedObject newValue;  ///< New value of the setting.
    };
  }

  class Settings;

  template<typename T>
  class Setting {
  public:
    Setting() = default;
    Setting(Settings * pSettings, bfc::String const & path)
      : m_path(path)
      , m_pSettings(pSettings) {}

    bfc::String const & path() const {
      return m_path;
    }

    T get(T const & defaultValue = {}) const {
      return m_pSettings ? m_pSettings->get<T>(path(), defaultValue) : defaultValue;
    }

    bool set(T const & val) const {
      if (m_pSettings == nullptr) {
        return false;
      }

      bfc::SerializedObject serialized;
      serialized.write(val);
      m_pSettings->setObject(path(), serialized);
      return true;
    }

  private:
    bfc::String    m_path;
    bfc::Settings* m_pSettings = nullptr;
  };

  class Events;
  class Settings {
  public:
    void attach(Events * pEvents);
    void detach(Events * pEvents);

    void add(bfc::StringView const & path, bfc::SerializedObject const & defaultValue = {});
    void add(bfc::StringView const & path, bool defaultValue = false);
    void add(bfc::StringView const & path, int64_t defaultValue = 0);
    void add(bfc::StringView const & path, double defaultValue = 0.0);
    void add(bfc::StringView const & path, bfc::StringView const & defaultValue = "");

    void setObject(bfc::StringView const & path, bfc::SerializedObject const & value);
    void set(bfc::StringView const & path, bool value);
    void set(bfc::StringView const & path, int64_t value);
    void set(bfc::StringView const & path, double value);
    void set(bfc::StringView const & path, bfc::StringView const & value);

    bfc::SerializedObject getObject(bfc::StringView const& path, bfc::SerializedObject const& defaultValue = {});
    bool                  getBool(bfc::StringView const& path, bool defaultValue = false);
    int64_t               getInt(bfc::StringView const& path, int64_t defaultValue = 0);
    double                getFloat(bfc::StringView const& path, double defaultValue = 0.0);
    bfc::String           getString(bfc::StringView const& path, bfc::StringView const& defaultValue = {});

    template<typename T>
    T get(bfc::StringView const & path, T const & defaultValue = {}) {
      bfc::SerializedObject s = getObject(path, ::bfc::serialize(defaultValue));
      bfc::Uninitialized<T> ret;
      if (!s.read(ret.get())) {
        return defaultValue;
      } else {
        return ret.get();
      }
    }

    /// Check if a setting exists.
    bool exists(bfc::StringView const & path);
    /// Remove settings from the settings manager.
    bool clear(bfc::StringView const & path, bool asGroup = false);
    /// Reset a setting to its default value
    bool reset(bfc::StringView const & path, bool asGroup = false);
    /// Call a function for a collection of settings.
    bool forEach(std::function<bool(bfc::StringView, bfc::SerializedObject)> const & callback, bfc::StringView const & root = {});
    /// Get paths for a collection of settings.
    bfc::Vector<bfc::String> enumerate(bfc::StringView const & root = {});

    /// Export settings.
    bfc::SerializedObject serialize(bfc::StringView const & root = {});
    /// Import settings in `settings`.
    bool deserialize(bfc::SerializedObject const & values);
    /// Export settings to a file.
    bool save(bfc::URI const & uri, bfc::StringView const & root = {}, bfc::DataFormat fmt = bfc::DataFormat_YAML);
    /// Imports settings from a file.
    bool load(bfc::URI const & uri, bfc::DataFormat fmt = bfc::DataFormat_YAML);

  private:
    Set<Events *> m_events;

    bfc::Timestamp m_lastChanged = bfc::Timestamp::now();
    bool           m_changed     = false;

    bfc::Map<bfc::String, bfc::SerializedObject> m_values;
    bfc::Map<bfc::String, bfc::SerializedObject> m_defaults;
  };
}
