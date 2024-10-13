#pragma once

#include "Span.h"
#include "Vector.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string_view>

namespace bfc {
  enum SplitFlags {
    SplitFlags_None,
    SplitFlags_DropEmpty,
    SplitFlags_KeepDelimiters,
  };

  class BFC_API StringView : public Span<char> {
  public:
    constexpr StringView()
      : StringView("", 0ll) {}

    StringView(char const * str);

    template<int64_t N>
    constexpr StringView(char const (&str)[N])
      : Span(str) {}

    constexpr StringView::StringView(char const * str, int64_t length)
      : Span(str, length) {}

    constexpr StringView::StringView(char const * first, char const * last)
      : Span<char>(first, last) {}

    constexpr StringView::StringView(StringView const & o)
      : StringView(o.begin(), o.end()) {}

    constexpr int64_t StringView::empty() const {
      return size() <= 0;
    }

    constexpr int64_t StringView::length() const {
      return math::max(0ll, size());
    }

    constexpr StringView StringView::substr(int64_t start, int64_t count = npos) const {
      start = std::clamp(start, 0ll, length());
      count = std::clamp(count, 0ll, length() - start); // Avoid overflow
      return StringView(data() + start, std::min(count, length() - start));
    }

    constexpr StringView StringView::trimEnd(StringView const & chars = " \r\n\t\v") const {
      int64_t end = findLastNotOf(chars);
      return end == npos ? StringView() : substr(0, end + 1);
    }

    constexpr StringView StringView::trimStart(StringView const & chars = " \r\n\t\v") const {
      int64_t start = findFirstNotOf(chars);
      return start == npos ? StringView() : substr(start, length() - start);
    }

    constexpr StringView StringView::trim(StringView const & chars = " \r\n\t\v") const {
      return trimEnd(chars).trimStart(chars);
    }

    constexpr int64_t StringView::find(char needle, int64_t start = 0) const {
      if (start >= length())
        return npos;
      for (int64_t offset = start; offset < length(); ++offset)
        if (at(offset) == needle)
          return offset;
      return npos;
    }

    constexpr int64_t StringView::find(StringView const & needle, int64_t start = 0) const {
      if (start >= length())
        return npos;
      for (int64_t offset = start; offset <= length() - needle.length(); ++offset)
        if (StringView(data() + offset, needle.length()) == needle)
          return offset;
      return npos;
    }

    constexpr int64_t StringView::find(Span<StringView> const & needles, int64_t start = 0, int64_t * pFoundIndex = nullptr) const {
      for (int64_t offset = start; offset < length(); ++offset) {
        int64_t remaining = length() - offset;
        for (auto & [i, needle] : enumerate(needles)) {
          if (needle.length() > remaining)
            continue;

          if (strncmp(begin() + offset, needle.begin(), needle.length()) != 0)
            continue;

          if (pFoundIndex != nullptr) {
            *pFoundIndex = i;
          }

          return offset;
        }
      }

      if (pFoundIndex != nullptr) {
        *pFoundIndex = -1;
      }

      return npos;
    }

    constexpr int64_t StringView::findLast(char needle, int64_t start = npos) const {
      for (int64_t offset = std::min(start, length() - 1); offset >= 0; --offset)
        if (at(offset) == needle)
          return offset;
      return npos;
    }

    constexpr int64_t StringView::findLast(StringView const & needle, int64_t start = npos) const {
      for (int64_t offset = std::min(start, length() - 1); offset >= 0; --offset)
        if (StringView(data() + offset, needle.length()) == needle)
          return offset;
      return npos;
    }

    constexpr int64_t StringView::findFirstOf(StringView const & needle, int64_t start = 0) const {
      for (int64_t offset = start; offset < length(); ++offset)
        if (needle.find(at(offset)) != npos)
          return offset;
      return npos;
    }

    constexpr int64_t StringView::findLastOf(StringView const & needle, int64_t start = npos) const {
      for (int64_t offset = std::min(start, length() - 1); offset >= 0; --offset)
        if (needle.find(at(offset)) != npos)
          return offset;
      return npos;
    }

    constexpr int64_t StringView::findFirstNotOf(char needle, int64_t start = 0) const {
      if (start >= length())
        return npos;
      for (char const & c : *this)
        if (c != needle)
          return &c - begin();
      return npos;
    }

    constexpr int64_t StringView::findFirstNotOf(StringView const & needle, int64_t start = 0) const {
      if (start >= length())
        return npos;
      for (int64_t offset = start; offset < length(); ++offset) {
        if (needle.find(at(offset)) == npos)
          return offset;
      }
      return npos;
    }

    constexpr int64_t StringView::findLastNotOf(char needle, int64_t start = npos) const {
      for (int64_t offset = std::min(start, length() - 1); offset >= 0; --offset)
        if (at(offset) != needle)
          return offset;
      return npos;
    }

    constexpr int64_t StringView::findLastNotOf(StringView const & needle, int64_t start = npos) const {
      for (int64_t offset = std::min(start, length() - 1); offset >= 0; --offset)
        if (needle.find(at(offset)) == npos)
          return offset;
      return npos;
    }

    constexpr bool StringView::startsWith(StringView const & match) const {
      return length() >= match.length() && substr(0, match.length()) == match;
    }

    constexpr bool StringView::endsWith(StringView const & match) const {
      return length() >= match.length() && substr(length() - match.length(), match.length()) == match;
    }

    bool equals(StringView const & with, bool ignoreCase = false) const;

    int compare(StringView const & with, bool ignoreCase = false) const;

    Vector<StringView> split(StringView const & sep, bool dropEmpty) const;

    Vector<StringView> split(Span<StringView> const & sep, bool dropEmpty) const;

    Vector<StringView> split(StringView const & sep, SplitFlags flags = SplitFlags_DropEmpty) const;

    Vector<StringView> split(Span<StringView> const & sep, SplitFlags flags = SplitFlags_DropEmpty) const;

    bool operator==(StringView const & rhs) const;

    bool operator!=(StringView const & rhs) const;

    bool operator<(StringView const & rhs) const;

    bool operator>(StringView const & rhs) const;

    bool operator<=(StringView const & rhs) const;

    bool operator>=(StringView const & rhs) const;
  };

  static const StringView whitespace = " \r\n\t";
  static const StringView numerals   = "0123456789";
} // namespace bfc

namespace std {
  template<>
  struct hash<bfc::StringView> {
    size_t operator()(const bfc::StringView & o) const {
      return std::hash<std::string_view>{}(std::string_view(o.begin(), o.length()));
    }
  };
} // namespace std
