#include "Application.h"
#include "core/URI.h"
#include "platform/OS.h"
#include "util/Log.h"

namespace engine {
  Application::Application(Options const & options)
    : bfc::Events(options.name)
    , m_options(options) {
    bfc::log.attach(this);
    m_settings.attach(this);
  }

  bool Application::parseCommandLine(int argc, char ** argv) {
    m_arguments.clear();
    for (int i = 0; i < argc; ++i) {
      m_arguments.pushBack(argv[i]);
    }

    m_subsystemLock.lock();
    auto systems = m_subsystems;
    m_subsystemLock.unlock();

    for (auto & pSystem : systems) {
      if (!pSystem->parseCommandLine(argc, argv)) {
        return false;
      }
    }

    return true;
  }

  bool Application::init() {
    m_settings.load(bfc::URI::File(getSettingsPath()));

    m_subsystemLock.lock();
    auto systems = m_subsystems;
    m_subsystemLock.unlock();

    for (auto & pSystem : systems) {
      if (!pSystem->init(this)) {
        return false;
      }
    }

    return true;
  }

  void Application::shutdown() {
    for (int64_t i = m_subsystems.size() - 1; i >= 0; --i) {
      m_subsystems[i]->shutdown();
    }
  }

  bfc::Ref<Subsystem> Application::findSubsystem(bfc::type_index const & type) const {
    std::scoped_lock guard{m_subsystemLock};

    return findSubsystem_unlocked(type);
  }

  bool Application::removeSubsystem(bfc::type_index const & type) {
    std::scoped_lock guard{m_subsystemLock};
    for (int64_t i = 0; i < m_subsystems.size(); ++i) {
      if (m_subsystems[i]->type == type) {
        m_subsystems.erase(i);
        return true;
      }
    }

    return false;
  }

  int Application::run() {
    m_lastFrameTime = bfc::Timestamp::now();
    while (m_running) {

      Events::update();

      for (int64_t i = 0; i < m_subsystems.size(); ++i) {
        m_subsystems[i]->loop(this);
      }
      auto now        = bfc::Timestamp::now();
      m_timestep      = now.length - m_lastFrameTime.length;
      m_lastFrameTime = now;
    }

    return m_exitCode;
  }

  void Application::exit(int code) {
    m_running  = false;
    m_exitCode = code;
  }

  bfc::StringView Application::getName() const {
    return m_options.name;
  }

  bfc::StringView Application::getCompany() const {
    return m_options.company;
  }

  bfc::Filename Application::getAppDataPath() const {
    return bfc::os::getSystemPath(bfc::os::FolderID_AppData) / (m_options.company + "/" + m_options.name);
  }

  bfc::Filename Application::getSettingsPath() const {
    return getAppDataPath() / "Settings";
  }

  bfc::Filename Application::getBinaryPath() const {
    return bfc::os::getExePath();
  }

  bfc::Filename Application::getWorkingDirectory() const {
    return bfc::os::getCwd();
  }

  bfc::Filename Application::getTempDirectory() const {
    return bfc::os::getSystemPath(bfc::os::FolderID_Temp) / getCompany() / getName();
  }

  bfc::Timestamp Application::getDeltaTime() const {
    return m_timestep;
  }

  bool Application::addSubsystem(bfc::Ref<Subsystem> const & pSystem) {
    std::scoped_lock guard{m_subsystemLock};
    if (findSubsystem_unlocked(pSystem->type) != nullptr) {
      return false;
    }

    pSystem->m_pApp = this;
    m_subsystems.pushBack(pSystem);
    return true;
  }

  bfc::Ref<Subsystem> Application::findSubsystem_unlocked(bfc::type_index const & type) const {
    for (auto & pExisting : m_subsystems) {
      if (pExisting->type == type) {
        return pExisting;
      }
    }

    return nullptr;
  }

  void Application::saveSettings() {
    bfc::Filename path = getSettingsPath();

    bfc::os::createFolders(path.parent());
    m_settings.save(bfc::URI::File(path));
  }
} // namespace engine
