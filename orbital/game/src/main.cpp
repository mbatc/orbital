#include "app/Orbital.h"
#include "util/Log.h"

int main(int argc, char ** argv) {
  Orbital orbital;
  auto logListener = orbital.addListener();
  logListener->on<bfc::events::AddLog>([](bfc::events::AddLog const & e) {
    const char * level = "INFO";
    switch (e.level) {
    case bfc::Log::Level_Info:    level = "INFO"; break;
    case bfc::Log::Level_Error:   level = "ERROR"; break;
    case bfc::Log::Level_Warning: level = "WARNING"; break;
    }
    printf("[%s][%.*s] %.*s (%.*s in %.*s() ln %lld)\n",
      level,
      (int)e.source.length(), e.source.begin(),
      (int)e.message.length(), e.message.begin(),
      (int)e.file.length(), e.file.begin(),
      (int)e.func.length(), e.func.begin(),
      e.line
    );
    return true;
  });

  if (!orbital.parseCommandLine(argc, argv)) {
    return 1;
  }

  if (!orbital.init()) {
    return 1;
  }

  int result = orbital.run();
  orbital.shutdown();
  return result;
}
