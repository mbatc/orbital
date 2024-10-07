#include "util/Scan.h"

namespace bfc {
  constexpr double powerOf5Table[20] = {0.000000512, 0.00000256, 0.0000128, 0.000064, 0.00032, 0.0016,  0.008,   0.04,     0.2,       1.0,
                                        5.0,         25.0,       125.0,     625.0,    3125.0,  15625.0, 78125.0, 390625.0, 1953125.0, 9765625.0};

  int64_t ScanIntegerFast(const char * begin, const char * end, int64_t * pLen) {
    const char * start = begin;
    int64_t      val   = 0;
    int          neg   = 0;

    while (begin < end && whitespace.find(*begin) != npos) // Skip whitespace
      ++begin;

    if (begin >= end || ((begin[0] < 48 || begin[0] > 57) && begin[0] != '-')) {
      if (pLen != nullptr) {
        *pLen = 0;
      }
      return 0;
    }

    if (begin[0] == '-') {
      neg = 1;
      ++begin;
    }

    for (; begin < end && begin[0] > 47 && begin[0] < 58; ++begin)
      val = (val << 1) + (val << 3) + begin[0] - 48;

    if (pLen)
      *pLen = begin - start;

    return neg ? -val : val;
  }

  double ScanDoubleFast(const char * begin, const char * end, int64_t * pLen) {
    const char * start = begin;

    while (begin < end && whitespace.find(*begin) != npos) // Skip whitespace
      ++begin;

    if (begin >= end || ((begin[0] < 48 || begin[0] > 57) && begin[0] != '-' && begin[0] != '.')) {
      if (pLen != nullptr) {
        *pLen = 0;
      }
      return 0;
    }

    int32_t sign = begin[0];
    begin += sign == '-' || sign == '+';

    int64_t sum        = 0;
    int32_t digitCount = 0;
    int32_t dpOffset   = -1;
    int32_t power10    = 0;
    int32_t dp         = '.';
    while (true) {
      while (begin < end && begin[0] >= '0' && begin[0] <= '9') {
        digitCount++;
        if (sum < LLONG_MAX / 10)
          sum = sum * 10 + begin[0] - '0';
        else
          ++power10;
        ++begin;
      }

      if (begin < end && begin[0] == dp) {
        dp       = '0';
        dpOffset = digitCount;
        ++begin;
      } else {
        break;
      }
    }

    if (dpOffset >= 0)
      power10 -= digitCount - dpOffset;

    if (begin[0] == 'e' || begin[0] == 'E') {
      ++begin;
      int32_t esign = begin[0];
      begin += esign == '-' || esign == '+';
      int32_t expo = 0;
      while (begin[0] >= '0' && begin[0] <= '9') {
        expo = expo * 10 + begin[0] - '0';
        ++begin;
      }

      if (esign == '-')
        expo = -expo;
      power10 += expo;
    }

    double value = (double)sum;
    if (power10) {
      // static lookup of 5^-9 to 5^10 to avoid calling pow()
      int32_t idx = power10 + 9;
      value *= idx >= 0 && idx < 20 ? powerOf5Table[idx] : pow(5, power10);
      value = ldexp(value, power10);
    }

    if (pLen)
      *pLen = begin - start;

    return sign == '-' ? -value : value;
  }

  StringView Scan::readString(const StringView & str, int64_t * pLen) {
    const int64_t len   = str.length();
    const int64_t start = str.findFirstNotOf(whitespace);
    const int64_t end   = str.findFirstOf(whitespace, start);

    if (start == npos)
      return StringView();

    StringView ret = str.substr(start, end - start);
    if (pLen)
      *pLen = ret.end() - str.begin();

    return ret;
  }

  double Scan::readFloat(const StringView & str, int64_t * pLen) {
    return ScanDoubleFast(str.begin(), str.end(), pLen);
  }

  bool Scan::readBool(const StringView & str, int64_t * pLen) {
    if (pLen)
      *pLen = 0;

    const int64_t len   = str.length();
    const int64_t start = str.findFirstNotOf(whitespace);
    if (start == npos)
      return false;

    int64_t    end   = str.findFirstOf(whitespace, start);
    StringView value = str.substr(start);
    bool       res   = false;
    if (value.equals("true", true)) {
      res = true;
    } else if (value.equals("false", true)) {
      res = false;
    } else if (numerals.findFirstOf(value.substr(0, 1)) == 0) {
      return readInt(value, pLen) != 0;
    } else {
      return false;
    }

    if (pLen)
      *pLen = value.end() - str.begin();

    return res;
  }

  StringView Scan::readQuote(StringView const & str, int64_t * pLen) {
    int64_t srclen = str.length();
    int64_t start  = str.findFirstOf("'\"", 0);
    if (start == npos)
      return {};

    char    terminator = str[start];
    int64_t end        = start + 1;
    bool    escaped    = false;
    while (end < str.length() && (str[end] != terminator || escaped)) {
      if (!escaped && str[end] == '\\')
        escaped = true;
      else
        escaped = false;
      ++end;
    }

    if (end >= str.length()) // Didn't find end quote
      return {};

    if (pLen)
      *pLen = end;

    return str.substr(start, end - start);
  }

  int64_t Scan::readInt(const StringView & str, int64_t * pLen) {
    return ScanIntegerFast(str.begin(), str.end(), pLen);
  }

  bool Scan::readBool(StringView * pStr) {
    int64_t len = 0;
    bool    ret = readBool(*pStr, &len);
    *pStr       = pStr->substr(len);
    return ret;
  }

  int64_t Scan::readInt(StringView * pStr) {
    int64_t len = 0;
    int64_t ret = readInt(*pStr, &len);
    *pStr       = pStr->substr(len);
    return ret;
  }

  double Scan::readFloat(StringView * pStr) {
    int64_t len = 0;
    double  ret = readFloat(*pStr, &len);
    *pStr       = pStr->substr(len);
    return ret;
  }

  StringView Scan::readString(StringView * pStr) {
    int64_t    len = 0;
    StringView ret = readString(*pStr, &len);
    *pStr          = pStr->substr(len);
    return ret;
  }

  StringView Scan::readQuote(StringView * pStr) {
    int64_t    len = 0;
    StringView ret = readQuote(*pStr, &len);
    *pStr          = pStr->substr(len);
    return ret;
  }
} // namespace bfc
