#include "util/Scan.h"
#include "framework/test.h"

using namespace bfc;

BFC_TEST(Scan_readInt) {
  StringView str = "10";
  int64_t    len = 0;

  BFC_TEST_ASSERT_TRUE(Scan::readInt(str, &len) == 10);
  BFC_TEST_ASSERT_TRUE(len == 2);
  str = " 15";
  BFC_TEST_ASSERT_TRUE(Scan::readInt(str, &len) == 15);
  BFC_TEST_ASSERT_TRUE(len == 3);

  str = "1234 ";
  BFC_TEST_ASSERT_TRUE(Scan::readInt(str, &len) == 1234);
  BFC_TEST_ASSERT_TRUE(len == 4);

  str = " abcd ";
  BFC_TEST_ASSERT_TRUE(Scan::readInt(str, &len) == 0);
  BFC_TEST_ASSERT_TRUE(len == 0);

  str = "abcd3";
  BFC_TEST_ASSERT_TRUE(Scan::readInt(str, &len) == 0);
  BFC_TEST_ASSERT_TRUE(len == 0);
}

BFC_TEST(Scan_readFloat) {
  StringView str = "10.5";
  int64_t    len = 0;

  BFC_TEST_ASSERT_TRUE(Scan::readFloat(str, &len) == 10.5);
  BFC_TEST_ASSERT_TRUE(len == 4);
  str = ".1";
  BFC_TEST_ASSERT_TRUE(Scan::readFloat(str, &len) == 0.1);
  BFC_TEST_ASSERT_TRUE(len == 2);

  str = "1234 ";
  BFC_TEST_ASSERT_TRUE(Scan::readFloat(str, &len) == 1234);
  BFC_TEST_ASSERT_TRUE(len == 4);

  str = " abcd ";
  BFC_TEST_ASSERT_TRUE(Scan::readFloat(str, &len) == 0);
  BFC_TEST_ASSERT_TRUE(len == 0);
}

BFC_TEST(Scan_readBool) {}

BFC_TEST(Scan_readString) {}

BFC_TEST(Scan_readQuote) {}
