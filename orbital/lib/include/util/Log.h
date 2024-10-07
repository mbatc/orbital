#pragma once

#include "../core/Set.h"
#include "../core/String.h"

namespace bfc {
  class Events;

  /// A logging utility class
  class BFC_API Log {
  public:
    enum Level {
      Level_Error,
      Level_Warning,
      Level_Info,
      Level_Count,
    };

    void attach(Events * pEvents);

    void detach(Events * pEvents);

    void write(StringView const & file, StringView const & func, int64_t line, Level const & level, StringView const & source, char const * message);

    template<typename... Args>
    void write(StringView const & file, StringView const & func, int64_t line, Level const & level, StringView const & source, char const * message,
               Args const &... args) {
      write(file, func, line, level, source, String::format(message, args...).c_str());
    }

  private:
    Set<Events *> m_events;
  };

  namespace events {
    struct AddLog {
      StringView file;
      StringView func;
      int64_t    line;
      Log::Level level;
      StringView source;
      StringView message;
    };
  }; // namespace events

  extern Log log;
} // namespace bfc

#define BFC_LOG_ERROR(src, message, ...)   ::bfc::log.write(BFC_FUNCTION, BFC_FUNCTION, BFC_LINE, bfc::Log::Level_Error, src, message, __VA_ARGS__)
#define BFC_LOG_WARNING(src, message, ...) ::bfc::log.write(BFC_FUNCTION, BFC_FUNCTION, BFC_LINE, bfc::Log::Level_Warning, src, message, __VA_ARGS__)
#define BFC_LOG_INFO(src, message, ...)    ::bfc::log.write(BFC_FUNCTION, BFC_FUNCTION, BFC_LINE, bfc::Log::Level_Info, src, message, __VA_ARGS__)
