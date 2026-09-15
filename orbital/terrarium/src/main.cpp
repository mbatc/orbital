#include "Terrarium.h"
#include "platform/OS.h"
#include "util/Log.h"
#include "mesh/MeshData.h"

int main(int argc, char ** argv) {
  Terrarium terrarium;

  auto logListener = terrarium.addListener();
  logListener->on([](bfc::events::AddLog const & e) {
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
  });

  if (!terrarium.parseCommandLine(argc, argv)) {
    return 1;
  }

  if (!terrarium.init()) {
    return 1;
  }

  const int result = terrarium.run();

  terrarium.shutdown();
  return result;
}
