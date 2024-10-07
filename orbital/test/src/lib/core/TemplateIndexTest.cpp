#include "framework/test.h"

#include "core/templateindex.h"
#include "core/StringView.h"
#include "core/Map.h"
#include "core/String.h"

using namespace bfc;

BFC_TEST(TemplateIndex_Equals) {
  BFC_TEST_ASSERT_EQUAL(templateid<Vector<String>>(), templateid<Vector<Vector<int64_t>>>());

  template_index mapInfo  = templateid<Map<int64_t, uint64_t>>();
  template_index map2Info  = templateid<Map<int64_t, Vector<double>>>();

  template_index pairInfo = templateid<Pair<int64_t, uint64_t>>();
  template_index pair2Info = templateid<Pair<int64_t, Vector<int64_t>>>();

  BFC_TEST_ASSERT_EQUAL(mapInfo, map2Info);
  BFC_TEST_ASSERT_EQUAL(pairInfo, pair2Info);

  BFC_TEST_ASSERT_NOT_EQUAL(mapInfo, pairInfo);
  BFC_TEST_ASSERT_NOT_EQUAL(mapInfo, pair2Info);
  BFC_TEST_ASSERT_NOT_EQUAL(map2Info, pairInfo);
  BFC_TEST_ASSERT_NOT_EQUAL(map2Info, pair2Info);
}

BFC_TEST(TemplateIndex_Name) {
  StringView templateName = detail::template_name<Vector<StringView>>::name();
  char const *    templateCStr = detail::template_name<Vector<StringView>>::c_str();

  BFC_TEST_ASSERT_EQUAL(String(templateName), String(templateCStr));
}

BFC_TEST(TemplateIndex_NoneTemplated) {
  StringView templateName = detail::template_name<StringView>::name();
  char const *    templateCStr = detail::template_name<StringView>::c_str();
  uint64_t        templateCode = detail::template_name<StringView>::hash_code();

  BFC_TEST_ASSERT_EQUAL(String(templateName), "");
  BFC_TEST_ASSERT_EQUAL(String(templateCStr), "");
  BFC_TEST_ASSERT_EQUAL(templateCode, 0);
}

BFC_TEST(TemplateIndex_hash) {
  hash(templateid<Vector<int64_t>>());
}
