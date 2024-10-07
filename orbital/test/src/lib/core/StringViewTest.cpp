#include "core/StringView.h"
#include "framework/test.h"

BFC_TEST(StringView_DefaultConstruct) {
  bfc::StringView a;
  BFC_TEST_ASSERT_TRUE(a.length() == 0);
  BFC_TEST_ASSERT_TRUE(a == "");
}

BFC_TEST(StringView_ConstructCString) {
  char const* testStr = "This is a string";
  bfc::StringView a = testStr;
  BFC_TEST_ASSERT_TRUE(a.length() == strlen(testStr));
  BFC_TEST_ASSERT_TRUE(a == testStr);
}

BFC_TEST(StringView_Compare) {
  bfc::StringView a;
  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}

BFC_TEST(StringView_Equals) {
  bfc::StringView a;
  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}

BFC_TEST(StringView_Find) {
  bfc::StringView a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.find("hello") == 0);
  BFC_TEST_ASSERT_TRUE(a.find("owo") == 4);
  BFC_TEST_ASSERT_TRUE(a.find("world") == 5);
  BFC_TEST_ASSERT_TRUE(a.find("nothing") == bfc::npos);
}

BFC_TEST(StringView_FindFirstOf) {
  bfc::StringView a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.findFirstOf("eor") == 1);
  BFC_TEST_ASSERT_TRUE(a.findFirstOf("eor", 2) == 4);
  BFC_TEST_ASSERT_TRUE(a.findFirstOf("eor", 5) == 6);
  BFC_TEST_ASSERT_TRUE(a.findFirstOf("z4tp") == bfc::npos);
  BFC_TEST_ASSERT_TRUE(a.findFirstOf("z4tp", 3) == bfc::npos);
}

BFC_TEST(StringView_FindLastOf) {
  bfc::StringView a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.findLastOf("eor") == 7);
  BFC_TEST_ASSERT_TRUE(a.findLastOf("eor", 2) == 1);
  BFC_TEST_ASSERT_TRUE(a.findLastOf("eor", 5) == 4);
  BFC_TEST_ASSERT_TRUE(a.findLastOf("z4tp") == bfc::npos);
  BFC_TEST_ASSERT_TRUE(a.findLastOf("z4tp", 3) == bfc::npos);
}

BFC_TEST(StringView_Find_Char) {
  bfc::StringView a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.find('e') == 1);
  BFC_TEST_ASSERT_TRUE(a.find('e', 2) == bfc::npos);

  BFC_TEST_ASSERT_TRUE(a.find('l') == 2);
  BFC_TEST_ASSERT_TRUE(a.find('l', 5) == 8);
}

BFC_TEST(StringView_FindLast_Char) {
  bfc::StringView a = "helloworld";

  BFC_TEST_ASSERT_TRUE(a.findLast('e') == 1);

  BFC_TEST_ASSERT_TRUE(a.findLast('l') == 8);
  BFC_TEST_ASSERT_TRUE(a.findLast('l', 5) == 3);
}

BFC_TEST(StringView_FindFirstNotOf) {
  bfc::StringView a = "helloworld";

  
  BFC_TEST_ASSERT_TRUE(a.findFirstNotOf("helo") == 5);
  BFC_TEST_ASSERT_TRUE(a.findFirstNotOf("helo", 6) == 7);
}

BFC_TEST(StringView_FindLastNotOf) {
  bfc::StringView a = "helloworld";

  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}

BFC_TEST(StringView_FindFirstNotOf_Char) {
  bfc::StringView a = "helloworld";

  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}

BFC_TEST(StringView_FindLastNotOf_Char) {
  bfc::StringView a = "helloworld";

  BFC_TEST_ASSERT_TRUE(("Test not implemented", false));
}
