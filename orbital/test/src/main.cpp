
#include "framework/test.h"
#include <typeindex>
#include <cstdio>

#include "core/Map.h"
#include "core/Span.h"
#include "core/String.h"
#include "core/StringView.h"
#include "core/Reflect.h"
#include "core/RuntimeType.h"
#include "mesh/parsers/FBXParser.h"
#include "core/URI.h"
#include "core/Stream.h"

int main(int argc, char **argv)
{
  if (!bfc::test::run())
  {
    printf("Press any enter to continue...\n");
    getchar();
    return 1;
  }

  return 0;
}
