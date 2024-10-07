#include "util/Log.h"
#include "platform/Events.h"

namespace bfc {
  bfc::Log log;

  void Log::attach(Events * pEvents) {
    m_events.add(pEvents);
  }

  void Log::detach(Events * pEvents) {
    m_events.erase(pEvents);
  }

  void Log::write(StringView const & file, StringView const & func, int64_t line, Level const & level, StringView const & source, char const * message) {
    events::AddLog logEvent;
    logEvent.level   = level;
    logEvent.file    = file;
    logEvent.func    = func;
    logEvent.line    = line;
    logEvent.source  = source;
    logEvent.message = message;

    for (Events* pEvent : m_events) {
      pEvent->broadcast(logEvent);
    }
  }
} // namespace bfc
