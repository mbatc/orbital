#include "core/URI.h"
#include "framework/test.h"
#include "core/Stream.h"

using namespace bfc;

BFC_TEST(URI_Relative) {
  URI relative = "file/path.thing";
  URI another  = "file:///D:/dev/orbital/orbital/game/src/engine/Assets/VirtualFileSystem.h";
  URI full     = "file:///D:/dev/orbital/orbital/game/assets";

  StringView path      = another.pathView();

  URI        resolved  = full.resolveRelativeReference(relative);
  URI        resolved2 = full.resolveRelativeReference(another);
  // auto       pString   = openURI(resolved2, FileMode_Read);
  // resolved2.path();

  BFC_TEST_ASSERT_EQUAL(resolved, "file:///D:/dev/orbital/orbital/game/assets/file/path.thing");
  BFC_TEST_ASSERT_EQUAL(resolved, "file:///D:/dev/orbital/orbital/game/src/engine/Assets/VirtualFileSystem.h");
}

BFC_TEST(URI_Schemes) {

}
