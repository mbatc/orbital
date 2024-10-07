#include "core/String.h"
#include "framework/test.h"

BFC_TEST(String_DefaultConstruct) {
  bfc::String a;
  BFC_TEST_ASSERT_TRUE(a.length() == 0);
  BFC_TEST_ASSERT_TRUE(a == "");
}

BFC_TEST(String_ConstructCString) {
  char const* testStr = "This is a string";
  bfc::String a = testStr;
  BFC_TEST_ASSERT_TRUE(a.length() == strlen(testStr));
  BFC_TEST_ASSERT_TRUE(a == testStr);
}

BFC_TEST(String_Compare) {
  bfc::String a;
  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}

BFC_TEST(String_Equals) {
  bfc::String a = "helloworld";
  bfc::String b = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.equals(b));
  BFC_TEST_ASSERT_TRUE(!a.equals("byebyeworld"));

  BFC_TEST_ASSERT_TRUE(a == b);
  BFC_TEST_ASSERT_TRUE(a != "byebyeworld");
}

BFC_TEST(String_Find) {
  bfc::String a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.find("hello") == 0);
  BFC_TEST_ASSERT_TRUE(a.find("owo") == 4);
  BFC_TEST_ASSERT_TRUE(a.find("world") == 5);
  BFC_TEST_ASSERT_TRUE(a.find("nothing") == bfc::npos);
}

BFC_TEST(String_FindFirstOf) {
  bfc::String a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.findFirstOf("eor") == 1);
  BFC_TEST_ASSERT_TRUE(a.findFirstOf("eor", 2) == 4);
  BFC_TEST_ASSERT_TRUE(a.findFirstOf("eor", 5) == 6);
  BFC_TEST_ASSERT_TRUE(a.findFirstOf("z4tp") == bfc::npos);
  BFC_TEST_ASSERT_TRUE(a.findFirstOf("z4tp", 3) == bfc::npos);
}

BFC_TEST(String_FindLastOf) {
  bfc::String a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.findLastOf("eor") == 7);
  BFC_TEST_ASSERT_TRUE(a.findLastOf("eor", 2) == 1);
  BFC_TEST_ASSERT_TRUE(a.findLastOf("eor", 5) == 4);
  BFC_TEST_ASSERT_TRUE(a.findLastOf("z4tp") == bfc::npos);
  BFC_TEST_ASSERT_TRUE(a.findLastOf("z4tp", 3) == bfc::npos);
}

BFC_TEST(String_Find_Char) {
  bfc::String a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.find('e') == 1);
  BFC_TEST_ASSERT_TRUE(a.find('e', 2) == bfc::npos);

  BFC_TEST_ASSERT_TRUE(a.find('l') == 2);
  BFC_TEST_ASSERT_TRUE(a.find('l', 5) == 8);
}

BFC_TEST(String_FindLast_Char) {
  bfc::String a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.findLast('e') == 1);

  BFC_TEST_ASSERT_TRUE(a.findLast('l') == 8);
  BFC_TEST_ASSERT_TRUE(a.findLast('l', 5) == 3);
}

BFC_TEST(String_FindFirstNotOf) {
  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}

BFC_TEST(String_FindLastNotOf) {
  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}

BFC_TEST(String_FindFirstNotOf_Char) {
  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}

BFC_TEST(String_FindLastNotOf_Char) {
  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}

BFC_TEST(String_FindLastNotOf_ToHex) {
  BFC_TEST_ASSERT_TRUE(bfc::toHex(1) == "00000001");
  BFC_TEST_ASSERT_TRUE(bfc::toHex(2) == "00000002");
  BFC_TEST_ASSERT_TRUE(bfc::toHex(3) == "00000003");
  BFC_TEST_ASSERT_TRUE(bfc::toHex(4) == "00000004");
  BFC_TEST_ASSERT_TRUE(bfc::toHex(128) == "00000080");
  BFC_TEST_ASSERT_TRUE(bfc::toHex(718237182) == "2ACF6DFE");
}

BFC_TEST(String_FindLastNotOf_FromHex) {
  BFC_TEST_ASSERT_TRUE(bfc::fromHex<uint32_t>("00000001") == 1);
  BFC_TEST_ASSERT_TRUE(bfc::fromHex<uint32_t>("1") == 1);
  BFC_TEST_ASSERT_TRUE(bfc::fromHex<uint32_t>("002") == 2);
  BFC_TEST_ASSERT_TRUE(bfc::fromHex<uint32_t>("0003") == 3);
  BFC_TEST_ASSERT_TRUE(bfc::fromHex<uint32_t>("4") == 4);
  BFC_TEST_ASSERT_TRUE(bfc::fromHex<uint32_t>("000080") == 128);
  BFC_TEST_ASSERT_TRUE(bfc::fromHex<uint32_t>("2ACF6DFE") == 718237182);
}
