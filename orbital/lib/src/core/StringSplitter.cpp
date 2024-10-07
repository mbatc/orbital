#include "core/StringSplitter.h"

namespace bfc {
  Vector<StringView> const & StringSplitter::operator()(StringView const & str, Span<StringView> const & delimList, const bool dropEmpty) {
    return (*this)(str, delimList, SplitFlags(SplitFlags_DropEmpty * dropEmpty));
  }

  Vector<StringView> const & StringSplitter::operator()(StringView const & str, Span<StringView> const & delimList, SplitFlags flags) {
    tokens.clear();

    bool dropEmpty = flags & SplitFlags_DropEmpty;
    bool keepDelim = flags & SplitFlags_KeepDelimiters;

    int64_t start = 0;
    int64_t end;
    do {
      int64_t delim = 0;
      end           = str.find(delimList, start, &delim);

      if (end != npos) {
        if (start != end || !dropEmpty)
          tokens.pushBack(str.substr(start, end - start));
        if (keepDelim)
          tokens.pushBack(delimList[delim]);
        start = end + delimList[delim].length();
      }

    } while (end != npos);

    if (start < str.length())
      tokens.pushBack(str.substr(start, str.length() - start));

    return tokens;
  }

  Vector<StringView> const & StringSplitter::operator()(StringView const & str, StringView const & delim, const bool dropEmpty) {
    return (*this)(str, Span<StringView>((StringView *)&delim, 1), dropEmpty);
  }

  Vector<StringView> const & StringSplitter::operator()(StringView const & str, StringView const & delim, SplitFlags flags) {
    return (*this)(str, Span<StringView>((StringView *)&delim, 1), flags);
  }
} // namespace bfc
