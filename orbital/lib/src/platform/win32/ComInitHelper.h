#pragma once
#include "core/Core.h"

#ifdef BFC_WINDOWS

namespace bfc {
  namespace win32 {
    struct ComInitHelper {
      ComInitHelper();
      ~ComInitHelper();
    };
  } // namespace win32
} // namespace bfc

#endif
