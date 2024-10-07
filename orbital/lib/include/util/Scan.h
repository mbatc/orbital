#pragma once

#include "../core/String.h"

namespace bfc {
  class BFC_API Scan {
  public:
    Scan() = delete;

    static bool       readBool(const StringView & str, int64_t * pLen = nullptr);
    static int64_t    readInt(const StringView & str, int64_t * pLen = nullptr);
    static double     readFloat(const StringView & str, int64_t * pLen = nullptr);
    static StringView readString(const StringView & str, int64_t * pLen = nullptr);
    static StringView readQuote(const StringView & str, int64_t * pLen = nullptr);

    static bool       readBool(StringView * pStr);
    static int64_t    readInt(StringView * pStr);
    static double     readFloat(StringView * pStr);
    static StringView readString(StringView * pStr);
    static StringView readQuote(StringView * pStr);
  };
} // namespace bfc
