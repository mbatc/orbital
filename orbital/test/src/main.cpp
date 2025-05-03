
#include "framework/test.h"
#include <typeindex>
#include <cstdio>

#include "scripting/WrenContext.h"
#include "platform/OS.h"
#include "platform/Events.h"
#include "util/Log.h"
#include "math/MathTypes.h"

int main(int argc, char **argv) {
  bfc::Events test("LogEvents");
  bfc::log.attach(&test);

  auto logListener = test.addListener();
  logListener->on([](bfc::events::AddLog const & e) {
    const char * level = "INFO";
    switch (e.level) {
    case bfc::Log::Level_Info: level = "INFO"; break;
    case bfc::Log::Level_Error: level = "ERROR"; break;
    case bfc::Log::Level_Warning: level = "WARNING"; break;
    }
    printf("[%s][%.*s] %.*s (%.*s in %.*s() ln %lld)\n", level, (int)e.source.length(), e.source.begin(), (int)e.message.length(), e.message.begin(),
           (int)e.file.length(), e.file.begin(), (int)e.func.length(), e.func.begin(), e.line);
  });

  bfc::scripting::wren::Module mod;
  bfc::scripting::wren::ForeignType & type = mod.add("Vec3");
  type
    .bind<bfc::Vec3>()
    .bind(bfc::ReflectConstructor<bfc::Vec3, float>{})
    .bind(bfc::ReflectConstructor<bfc::Vec3, float, float, float>{});

  bfc::URI engineModule = bfc::URI::File("D:/dev/orbital/build/bin/test.wren");

  bfc::scripting::WrenContext ctx;
  ctx.addForeignModule(bfc::URI::File("D:/dev/orbital/build/bin/test.wren"), mod);
  ctx.interpret(bfc::URI::File(bfc::os::getCwd()), R"(
import "test"

System.print("I am running in a VM!")
)");

  auto                        engineClass            = ctx.getVariable("Engine", engineModule);
  auto                        utilClass              = ctx.getVariable("Util", engineModule);
  bfc::scripting::wren::Value engineAttributes       = engineClass["attributes"];
  bfc::scripting::wren::Value engineMethodAttributes = engineAttributes["methods"];

  printf("Engine:\n");
  for (auto & method : engineClass.methods()) {
    printf("[ ");
    switch (method.type) {
    case bfc::scripting::wren::MethodType_Constructor: printf("ctor  "); break;
    case bfc::scripting::wren::MethodType_Getter:      printf("getter"); break;
    case bfc::scripting::wren::MethodType_Setter:      printf("setter"); break;
    case bfc::scripting::wren::MethodType_Method:      printf("method"); break;
    }

    auto serializeAttribute = engineMethodAttributes[method.symbol][nullptr]["serialize"];
    bool serialize = method.type == bfc::scripting::wren::MethodType_Getter && serializeAttribute[0].get<bool>().value_or(true);

    const char * methodAttribs = utilClass["mapToString"](engineMethodAttributes[method.symbol]).get<const char *>().value_or("null");

    printf(" ] %.*s (attrib: %s)\n", (int)method.signature.length(), method.signature.data(), methodAttribs);
  }

  if (!bfc::test::run())
  {
    printf("Press any enter to continue...\n");
    getchar();
    return 1;
  }

  return 0;
}
