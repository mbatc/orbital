#include "core/Core.h"
#include <cassert>
#include "platform/Window.h"

namespace bfc {
  bool assertion(char const * file, char const * function, int64_t line, bool condition, char const * expression, char const * message, ...) {
    constexpr int64_t MaxMessageLength = 1024;
    constexpr int64_t MaxAssertLength  = 2 * MaxMessageLength;
    if (!condition) {
      char messageBuffer[MaxMessageLength]  = {};
      char assertionBuffer[MaxAssertLength] = {};

      char assertionFmt[] = {"%s\n\n"
                             "Expression \"%s\" is False.\n\n"
                             "Function: %s\n"
                             "File: %s:%lld"};

      va_list l;
      va_start(l, message);
      vsprintf_s(messageBuffer, MaxMessageLength, message, l);
      va_end(l);

      sprintf_s(assertionBuffer, MaxAssertLength, assertionFmt, message, expression, function, file, line);

      switch (platform::errorMessageBox("Assertion Failed", messageBuffer)) {
      case platform::MessageBoxButton_Abort: exit(1); break;
      case platform::MessageBoxButton_Retry: __debugbreak(); break;
      case platform::MessageBoxButton_Ignore: return true;
      }
    }

    return condition;
  }

  bool fail(char const * file, char const * function, int64_t line, char const * message, ...) {
    constexpr int64_t MaxMessageLength = 1024;
    constexpr int64_t MaxAssertLength  = 2 * MaxMessageLength;

    char messageBuffer[MaxMessageLength] = {};
    char failureBuffer[MaxAssertLength]   = {};

    char failureFormat[] = {"%s\n\n"
                           "Function: %s\n"
                           "File: %s:%lld"};

    va_list l;
    va_start(l, message);
    vsprintf_s(messageBuffer, MaxMessageLength, message, l);
    va_end(l);

    sprintf_s(failureBuffer, MaxAssertLength, failureFormat, message, function, file, line);

    switch (platform::errorMessageBox("Assertion Failed", messageBuffer)) {
    case platform::MessageBoxButton_Abort: exit(1); break;
    case platform::MessageBoxButton_Retry: __debugbreak(); break;
    case platform::MessageBoxButton_Ignore: return true;
    }
  }
} // namespace bfc
