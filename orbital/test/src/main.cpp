
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

#include <variant>

#include <stdio.h>

#include "core/SerializedObject.h"

struct DefaultContext {};

struct SpecialContext {
  int64_t val = 10;
};

struct NeedsSpecialContext {
  bfc::String a;
};

template<typename T, typename Context = DefaultContext>
bfc::SerializedObject writer(T const & o, Context const & ctx = {}) {
  static_assert(false, "Not implemented");
}

template<>
bfc::SerializedObject writer<NeedsSpecialContext, SpecialContext>(NeedsSpecialContext const & o, SpecialContext const & ctx) {
  printf(o.a.c_str());
  return bfc::SerializedObject::MakeText(o.a + bfc::toString(ctx.val));
}

template<typename T, typename Context = DefaultContext>
bfc::SerializedObject writer(bfc::Vector<T> const & o, Context const & ctx = {}) {
  return bfc::SerializedObject::MakeArray(o.map([&](auto & i) { return writer(i, ctx); }));
}

int main(int argc, char **argv)
{
  bfc::Vector<NeedsSpecialContext> test = {{"hello"}, {" "}, {" world"}};

  auto res = writer(test, SpecialContext{});

  if (!bfc::test::run())
  {
    printf("Press any enter to continue...\n");
    getchar();
    return 1;
  }

  return 0;
}
