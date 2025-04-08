
#include "framework/test.h"
#include <typeindex>
#include <cstdio>


int main(int argc, char **argv)
{
  auto res = writer(test, SpecialContext{});

  if (!bfc::test::run())
  {
    printf("Press any enter to continue...\n");
    getchar();
    return 1;
  }

  return 0;
}
