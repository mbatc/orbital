#include "ComInitHelper.h"

#ifdef BFC_WINDOWS

#include <windows.h>

namespace bfc {
  namespace win32 {
    static thread_local int64_t s_comInitStack = 0;
    static thread_local HRESULT s_comInitResult = E_FAIL;

    ComInitHelper::ComInitHelper() {
      if (s_comInitStack++ == 0) {
        s_comInitResult = CoInitializeEx(0, 0);
        BFC_ASSERT(SUCCEEDED(s_comInitResult), "CoInitialize Failed");
      }
    }

    ComInitHelper::~ComInitHelper() {
      if (--s_comInitStack == 0 && SUCCEEDED(s_comInitResult)) {
        CoUninitialize();
        s_comInitResult = E_FAIL;
      }
    }
  } // namespace win32
} // namespace bfc

#endif
