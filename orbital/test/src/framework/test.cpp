#include "test.h"

#include <vector>
#include <functional>
#include <string>

namespace bfc
{
  namespace test
  {
    struct Test
    {
      std::function<void()> func;
      std::string name;
      std::string failureMessage;
      bool failed = false;
    };

    std::vector<Test>& GetTests()
    {
      static std::vector<Test> tests;
      return tests;
    }
    thread_local Test * g_pRunningTest = nullptr;

    TestRegister::TestRegister(char const *name, TestFunction func)
    {
      Test newTest;
      newTest.name = name;
      newTest.func = func;
      GetTests().push_back(newTest);
    }

    void failTest(char const * message)
    {
      g_pRunningTest->failureMessage = message;
      g_pRunningTest->failed = true;
    }

    bool assertTest(bool condition, char const * message)
    {
      if (!condition)
        failTest(message);
      return condition;
    }

    bool run()
    {
      bool anyFailed = false;
      int64_t maxNameLength = 0;
      for (Test &test : GetTests())
        maxNameLength = std::max<int64_t>(maxNameLength, test.name.length());

      int64_t numTests = GetTests().size();
      int numSize = (int)floor(log10(numTests)) + 1;

      int64_t i = 0;
      for (Test &test : GetTests())
      {
        g_pRunningTest = &test;
        printf("[%*lld/%*lld] Running %s", numSize, ++i, numSize, numTests, g_pRunningTest->name.c_str());
        printf("%*s ", (int)(maxNameLength - g_pRunningTest->name.length()), "");

        g_pRunningTest->func();

        if (g_pRunningTest->failed)
          printf("[ FAILED ] %s\n", g_pRunningTest->failureMessage.c_str());
        else
          printf("[ PASSED ]\n");

        anyFailed |= g_pRunningTest->failed;
      }

      return !anyFailed;
    }
  }
}
