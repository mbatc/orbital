#include "core/StringView.h"
#include "core/StringSplitter.h"
#include "util/Iterators.h"

namespace bfc {
  StringView::StringView(char const * str)
    : Span(str, strlen(str)) {}

  bool StringView::equals(StringView const & with, bool ignoreCase) const {
    return length() == with.length() && compare(with, ignoreCase) == 0;
  }

  int StringView::compare(StringView const & with, bool ignoreCase) const {
    int result = ignoreCase ? _strnicmp(data(), with.data(), std::min(length(), with.length()))
                            : strncmp(data(), with.data(), std::min(length(), with.length()));
    if (result != 0)
      return result;
    if (length() < with.length())
      return -1;
    if (length() > with.length())
      return 1;
    return 0;
  }

  Vector<StringView> StringView::split(StringView const & sep, bool dropEmpty) const {
    return StringSplitter()(*this, sep, dropEmpty);
  }

  Vector<StringView> StringView::split(Span<StringView> const & sep, bool dropEmpty) const {
    return StringSplitter()(*this, sep, dropEmpty);
  }

  Vector<StringView> StringView::split(StringView const & sep, SplitFlags flags) const {
    return StringSplitter()(*this, sep, flags);
  }

  Vector<StringView> StringView::split(Span<StringView> const & sep, SplitFlags flags) const {
    return StringSplitter()(*this, sep, flags);
  }

  bool StringView::operator==(StringView const & rhs) const {
    return equals(rhs);
  }

  bool StringView::operator!=(StringView const & rhs) const {
    return !equals(rhs);
  }

  bool StringView::operator<(StringView const & rhs) const {
    return compare(rhs) < 0;
  }

  bool StringView::operator>(StringView const & rhs) const {
    return compare(rhs) > 0;
  }

  bool StringView::operator<=(StringView const & rhs) const {
    return !(*this > rhs);
  }

  bool StringView::operator>=(StringView const & rhs) const {
    return !(*this < rhs);
  }
} // namespace bfc