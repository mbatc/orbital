#pragma once

namespace bfc {
  namespace test {
    typedef void (*TestFunction)();

    class TestRegister {
    public:
      TestRegister(char const * testName, TestFunction testFunc);
    };

    void failTest(char const * message);
    bool assertTest(bool condition, char const * message);

    bool run();
  } // namespace test
} // namespace bfc

#define BFC_TEST_ASSERT_EQUAL(a, b)                                                                                                                             \
  do {                                                                                                                                                         \
    bool ok = ((a) == (b));                                                                                                                                     \
    if (!::bfc::test::assertTest(ok, "Assertion Failed (" BFC_FILE ":" BFC_TOSTRING(BFC_LINE) "): " #a " == " #b ""))                                          \
      return;                                                                                                                                                  \
  } while (false)

#define BFC_TEST_ASSERT_NOT_EQUAL(a, b)                                                                                                                            \
  do {                                                                                                                                                         \
    bool ok = ((a) != (b));                                                                                                                                    \
    if (!::bfc::test::assertTest(ok, "Assertion Failed (" BFC_FILE ":" BFC_TOSTRING(BFC_LINE) "): " #a " != " #b ""))                                          \
      return;                                                                                                                                                  \
  } while (false)

#define BFC_TEST_ASSERT_TRUE(condition)                                                                                                                        \
  do {                                                                                                                                                         \
    bool ok = (condition);                                                                                                                                     \
    if (!::bfc::test::assertTest(ok, "Assertion Failed (" BFC_FILE ":" BFC_TOSTRING(BFC_LINE) "): (" #condition ") == false (expected true)"))                 \
      return;                                                                                                                                                  \
  } while (false)

#define BFC_TEST_ASSERT_FALSE(condition)                                                                                                                       \
  do {                                                                                                                                                         \
    bool ok = (condition);                                                                                                                                     \
    if (!::bfc::test::assertTest(!ok, "Assertion Failed (" BFC_FILE ":" BFC_TOSTRING(BFC_LINE) "): (" #condition ") == true (expected false"))                 \
      return;                                                                                                                                                  \
  } while (false)

#define BFC_TEST_FAIL(message)                                                                                                                                 \
  do {                                                                                                                                                         \
    ::bfc::test::failTest(message);                                                                                                                            \
    if (true)                                                                                                                                                  \
      return;                                                                                                                                                  \
  } while (false)

#define BFC_TEST(Name)                                                                                                                                         \
  void                                           __bfc_testfunc_##Name();                                                                                      \
  __declspec(dllexport)::bfc::test::TestRegister __bfc_test_##Name(#Name, __bfc_testfunc_##Name);                                                              \
  void                                           __bfc_testfunc_##Name()
