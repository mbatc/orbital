#pragma once

#include "core/Vector.h"
#include "core/typeindex.h"
#include "util/Settings.h"
#include "Subsystem.h"

#include <mutex>

namespace engine {
  class Subsystem;
  class Application : public bfc::Events {
  public:
    struct Options {
      bfc::String company = "DefaultCompany";
      bfc::String name    = "DefaultApp";
    };

    Application(Options const & options);

    bool parseCommandLine(int argc, char **argv);

    bool init();

    void shutdown();

    int run();

    void exit(int code = 0);

    bfc::StringView getName() const;

    bfc::StringView getCompany() const;

    bfc::Filename getAppDataPath() const;

    bfc::Filename getSettingsPath() const;
    
    bfc::Filename getBinaryPath() const;

    bfc::Filename getWorkingDirectory() const;

    bfc::Filename getTempDirectory() const;

    bfc::Timestamp getDeltaTime() const;

    template<typename SubsystemType, typename... Args>
    bfc::Ref<SubsystemType> addSubsystem(Args&&... args) {
      auto pSystem = bfc::NewRef<SubsystemType>(std::forward<Args>(args)...);
      if (!addSubsystem(pSystem)) {
        return nullptr;
      }
      return pSystem;
    }

    template<typename SubsystemType>
    bfc::Ref<SubsystemType> findSubsystem() const {
      return std::static_pointer_cast<SubsystemType>(findSubsystem(bfc::TypeID<SubsystemType>()));
    }

    bfc::Ref<Subsystem> findSubsystem(bfc::type_index const& type) const;

    template<typename SubsystemType>
    bool removeSubsystem() {
      return removeSubsystem(bfc::TypeID<SubsystemType>());
    }

    bool removeSubsystem(bfc::type_index const& type);

    template<typename T>
    bfc::Setting<T> getSetting(bfc::StringView const & name) {
      return bfc::Setting<T>(&m_settings, name);
    }

    template<typename T>
    bfc::Setting<T> addSetting(bfc::StringView const & name, T const defaultValue = {}) {
      m_settings.add(name, bfc::serialize(defaultValue));
      return getSetting<T>(name);
    }

    void saveSettings();

  private:
    bool addSubsystem(bfc::Ref<Subsystem> const& pSystem);
    bfc::Ref<Subsystem> findSubsystem_unlocked(bfc::type_index const & type) const;

    Options m_options;

    // Program running state
    bool m_running = true;
    int m_exitCode = 0;

    bfc::Timestamp m_timestep;
    bfc::Timestamp m_lastFrameTime;

    // Arguments parsed to the application
    bfc::Vector<bfc::String> m_arguments; 

    // Registered subsystems
    mutable std::mutex m_subsystemLock;
    bfc::Vector<bfc::Ref<Subsystem>> m_subsystems;

    bfc::Settings m_settings;
  };
}
