#include "framework/test.h"
#include "core/Array.h"

using namespace bfc;

BFC_TEST(Array) {
  bfc::Array<char, 10> arr = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

  auto spliced = arr.splice<1, 3>();
}
