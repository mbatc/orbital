
#include "framework/test.h"
#include "util/Iterators.h"
#include "core/Vector.h"
#include "core/Map.h"

using namespace bfc;

BFC_TEST(Util_Common_EnumerateAny) {
  Vector<int64_t> indices = { 0, 1, 2, 3, 4 };
  for (auto& [i, rhsVal] : enumerate(indices)) {
    BFC_TEST_ASSERT_TRUE(rhsVal == i); // The index is correct
    BFC_TEST_ASSERT_TRUE(rhsVal == indices[i]); // They have the same value
    BFC_TEST_ASSERT_TRUE(&rhsVal == &indices[i]); // They are the same object
  }


  Vector<int64_t> original = { 1, 2, 3, 4, 5 };
  Vector<int64_t> ints = { 1, 2, 3, 4, 5 };
  for (auto& [i, rhsVal] : enumerate(ints)) {
    rhsVal = rhsVal * 2;
  }

  for (int64_t i = 0; i < ints.size(); ++i) {
    BFC_TEST_ASSERT_TRUE(ints[i] == original[i] * 2);
  }
}

BFC_TEST(Util_Common_EnumerateItr) {
  Vector<int64_t> original = { 1, 2, 3, 4, 5 };
  Vector<int64_t> ints = { 1, 2, 3, 4, 5 };
  for (auto& [i, rhsVal] : enumerate(ints.begin(), ints.end())) {
    rhsVal = rhsVal * 2;
  }

  for (int64_t i = 0; i < ints.size(); ++i) {
    BFC_TEST_ASSERT_TRUE(ints[i] == original[i] * 2);
  }
}
