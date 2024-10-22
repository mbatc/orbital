#include "Subsystem.h"
#include "Application.h"

using namespace bfc;

namespace engine {
  Events * Subsystem::getEvents() {
    return m_pApp;
  }

  Events const * Subsystem::getEvents() const {
    return m_pApp;
  }

  Application * Subsystem::getApp() const {
    return m_pApp;
  }
} // namespace engine
