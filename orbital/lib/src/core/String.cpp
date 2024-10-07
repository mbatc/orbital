#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "core/String.h"
#include "core/Stream.h"

#include <algorithm>
#include <codecvt>
#include <locale>
#include <string>

namespace bfc {
  String::String() {
    m_data.pushBack(0);
  }

  String::String(char const * str)
    : String(str, strlen(str)) {}

  String::String(char const * first, char const * last) {

#ifdef BFC_DEBUG
    char const * it = first;
    while (it < last) {
      BFC_ASSERT(*it != 0, "Stray null terminator found");
      ++it;
    }
#endif

    m_data.pushBack(first, last);
    m_data.pushBack(0);
  }

  String::String(char const * str, int64_t length)
    : String(str, str + length) {}

  String::String(Span<char> const & data)
    : String(data.begin(), data.end()) {}

  String::String(Vector<char> && data)
    : m_data(std::move(data)) {
    if (m_data.empty())
      m_data.pushBack(0);

    BFC_ASSERT(m_data.back() == '\0', "Vector moved into string without a null terminator");
    if (m_data.back() != '\0')
      m_data.pushBack('\0');
  }

  StringView String::substr(int64_t start, int64_t count) const {
    count       = std::max(0ll, std::min(count, length()));
    int64_t end = std::min(length(), start + count);
    return StringView((char *)data() + start, (char *)data() + end);
  }

  StringView String::getView() const {
    return StringView((char *)data(), length());
  }

  int64_t String::length() const {
    return m_data.size() - 1;
  }

  bool String::empty() const {
    return length() == 0;
  }

  bool String::equals(StringView const & view, bool ignoreCase) const {
    return compare(view, ignoreCase) == 0;
  }

  int String::compare(StringView const & with, bool ignoreCase) const {
    return getView().compare(with, ignoreCase);
  }

  String String::concat(StringView const & rhs) const {
    String ret = *this;
    ret.pushBack(rhs);
    return ret;
  }

  String String::replace(StringView const & match, StringView const & replace, int64_t start) const {
    String ret;
    ret.reserve(length());

    size_t last = 0;
    size_t pos  = find(match, start);

    while (pos != npos) {
      ret.insert(ret.length(), substr(last, pos - last));
      ret.insert(ret.length(), replace);
      last = pos + match.size();
      pos  = find(match, last);
    }

    ret.insert(ret.length(), substr(last, pos - last));

    return ret;
  }

  String String::replacePart(StringView const & section, StringView const & replace) const {
    return StringView(begin(), section.begin()) + replace + StringView(section.end(), end());
  }

  String String::escaped() const {
    Vector<char> ret;

    ret.reserve(m_data.size() * 5 / 4);

    for (auto &[i, c] : enumerate(m_data)) {
      if (c >= 32 && c < 127) {
        ret.pushBack(c);
        continue;
      }

      if (c == '\0' && i + 1 == m_data.size())
        break; // Only break if it is the last character. Encode nulls within the string, but not the final terminator.

      switch (c) {
      case '\\': ret.pushBack({'\\', '\\'}); break;
      case '\a': ret.pushBack({'\\', 'a'}); break;
      case '\b': ret.pushBack({'\\', 'b'}); break;
      case '\f': ret.pushBack({'\\', 'f'}); break;
      case '\n': ret.pushBack({'\\', 'n'}); break;
      case '\r': ret.pushBack({'\\', 'r'}); break;
      case '\t': ret.pushBack({'\\', 't'}); break;
      case '\v': ret.pushBack({'\\', 'v'}); break;
      default: {
        auto hexCode = toHex(c);
        ret.pushBack({'\\', 'x'});
        ret.pushBack(hexCode);
        break;
      }
      }
    }

    ret.pushBack(0);
    return ret;
  }

  String String::unescaped() const {
    Vector<char> ret;
    ret.reserve(m_data.size());

    bool inEscapeSequence = false;
    for (int64_t i = 0; i < m_data.size(); ++i) {
      char c = m_data[i];
      if (inEscapeSequence) {
        switch (c) {
        case '\\': c = '\\'; break;
        case 'a':  c = '\a'; break;
        case 'b':  c = '\b'; break;
        case 'f':  c = '\f'; break;
        case 'n':  c = '\n'; break;
        case 'r':  c = '\r'; break;
        case 't':  c = '\t'; break;
        case 'v':  c = '\v'; break;
        case 'x':  // Fall-through
        case 'u': {
          int64_t start = i + 1;
          for (i = start; i < m_data.size() && isHex(m_data[i]); ++i) {}
          c     = fromHex<char>({m_data.begin() + start, 4});
          start = i;
          ret.pushBack(c);
        }
        }
      } else if (c == '\\') {
        inEscapeSequence = true;
        continue;
      }

      ret.pushBack(c);
    }
    return ret;
  }

  StringView String::trim(StringView const & chars) const {
    int64_t start = findFirstNotOf(chars);
    int64_t end   = findLastNotOf(chars);
    if (start == npos || end == npos)
      return "";
    return substr(start, end + 1 - start);
  }

  String String::toLower() const {
    String ret;
    ret.reserve(length());
    for (char c : *this)
      ret.pushBack((char)std::tolower(c));
    return ret;
  }

  String String::toUpper() const {
    String ret;
    ret.reserve(length());
    for (char c : *this)
      ret.pushBack((char)std::toupper(c));
    return ret;
  }

  Vector<StringView> String::split(Span<StringView> const & sep, bool dropEmtpy) const {
    return getView().split(sep, dropEmtpy);
  }

  Vector<StringView> String::split(StringView const & sep, bool dropEmtpy) const {
    return getView().split(sep, dropEmtpy);
  }

  Vector<StringView> String::split(Span<StringView> const & sep, SplitFlags flags) const {
    return getView().split(sep, flags);
  }

  Vector<StringView> String::split(StringView const & sep, SplitFlags flags) const {
    return getView().split(sep, flags);
  }

  String String::join(Span<String> const & strings, StringView const & sep, bool ignoreEmpty) {
    int64_t totalSize = 0;
    for (const String & str : strings)
      totalSize += str.length() + sep.length();
    String ret;
    ret.reserve(totalSize);
    for (const String & str : strings) {
      if (!ignoreEmpty || str.length() > 0) {
        ret.pushBack(str);
        if (&str - &strings.back())
          ret.pushBack(sep);
      }
    }
    return ret;
  }

  String String::join(Span<StringView> const & strings, StringView const & sep, bool ignoreEmpty) {
    int64_t totalSize = 0;
    for (const StringView & str : strings)
      totalSize += str.length() + sep.length();
    String ret;
    ret.reserve(totalSize);
    for (const StringView & str : strings) {
      if (!ignoreEmpty || str.length() > 0) {
        ret.pushBack(str);
        if (&str - &strings.back())
          ret.pushBack(sep);
      }
    }
    return ret;
  }

  int64_t String::find(char needle, int64_t start) const {
    return getView().find(needle, start);
  }

  int64_t String::find(StringView const & needle, int64_t start) const {
    return getView().find(needle, start);
  }

  int64_t String::findLast(char needle, int64_t start) const {
    return getView().findLast(needle, start);
  }

  int64_t String::findLast(StringView const & needle, int64_t start) const {
    return getView().findLast(needle, start);
  }

  int64_t String::findFirstOf(StringView const & needle, int64_t start) const {
    return getView().findFirstOf(needle, start);
  }

  int64_t String::findLastOf(StringView const & needle, int64_t start) const {
    return getView().findLastOf(needle, start);
  }

  int64_t String::findFirstNotOf(char needle, int64_t start) const {
    return getView().findFirstNotOf(needle, start);
  }

  int64_t String::findFirstNotOf(StringView const & needle, int64_t start) const {
    return getView().findFirstNotOf(needle, start);
  }

  int64_t String::findLastNotOf(char needle, int64_t start) const {
    return getView().findLastNotOf(needle, start);
  }

  int64_t String::findLastNotOf(StringView const & needle, int64_t start) const {
    return getView().findLastNotOf(needle, start);
  }

  bool String::startsWith(StringView const & match) const {
    return getView().startsWith(match);
  }

  bool String::endsWith(StringView const & match) const {
    return getView().endsWith(match);
  }

  int64_t String::capacity() const {
    return m_data.capacity() - 1;
  }

  bool String::reserve(int64_t newCapacity) {
    return m_data.reserve(newCapacity + 1);
  }

  void String::resize(int64_t newLength, char c) {
    int64_t nullPos = length();
    m_data.resize(newLength + 1, c);
    std::swap(m_data[nullPos], m_data.back());
  }

  void String::insert(int64_t index, char c) {
    m_data.insert(index, c);
  }

  void String::insert(int64_t index, char const * data, int64_t count) {
    m_data.insert(index, data, data + count);
  }

  void String::insert(int64_t index, StringView const & data) {
    m_data.insert(index, data.begin(), data.end());
  }

  void String::pushBack(char c) {
    insert(length(), c);
  }

  void String::pushBack(char const * data, int64_t count) {
    insert(length(), data, count);
  }

  void String::pushBack(StringView const & data) {
    insert(length(), data);
  }

  void String::erase(int64_t index, int64_t count) {
    m_data.erase(index, count);
  }

  void String::pushFront(char c) {
    insert(0, c);
  }

  void String::pushFront(char const * data, int64_t count) {
    insert(0, data, count);
  }

  void String::pushFront(StringView const & data) {
    insert(0, data);
  }

  char String::popBack() {
    char c = m_data[length() - 1];
    m_data.erase(c);
    return c;
  }

  char String::popFront() {
    return m_data.popFront();
  }

  char * String::c_str() {
    return data();
  }

  char const * String::c_str() const {
    return data();
  }

  char * String::begin() {
    return m_data.begin();
  }
  char * String::end() {
    return m_data.begin() + length();
  }
  char * String::data() {
    return m_data.data();
  }

  char const * String::begin() const {
    return m_data.begin();
  }
  char const * String::end() const {
    return m_data.begin() + length();
  }
  char const * String::data() const {
    return m_data.data();
  }

  String::operator StringView() const {
    return getView();
  }

  char & String::operator[](int64_t index) {
    return m_data[index];
  }

  char const & String::operator[](int64_t index) const {
    return m_data[index];
  }

  bool String::operator==(StringView const & rhs) const {
    return equals(rhs);
  }

  bool String::operator!=(StringView const & rhs) const {
    return !equals(rhs);
  }

  bool String::operator<(StringView const & rhs) const {
    return compare(rhs) < 0;
  }

  bool String::operator>(StringView const & rhs) const {
    return compare(rhs) > 0;
  }

  bool String::operator<=(StringView const & rhs) const {
    return !(*this > rhs);
  }

  bool String::operator>=(StringView const & rhs) const {
    return !(*this < rhs);
  }

  String String::operator+(StringView const & rhs) const {
    return concat(rhs);
  }

  String String::operator+(String const & rhs) const {
    return concat(rhs);
  }

  String String::operator+(char const * rhs) const {
    return concat(rhs);
  }

  String operator+(StringView const & lhs, String const & rhs) {
    return String(lhs) + rhs;
  }

  String operator+(char const * lhs, String const & rhs) {
    return String(lhs) + rhs;
  }

  String operator+(StringView const & lhs, StringView const & rhs) {
    return String(lhs) + rhs;
  }

  String operator+(StringView const& lhs, char const* rhs) {
    return String(lhs) + rhs;
  }

  String toString(double value) {
    return std::to_string(value).c_str();
  }

  String toString(int64_t value) {
    return std::to_string(value).c_str();
  }

  int64_t write(Stream * pStream, String const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!pStream->write(pValue[i].m_data)) {
        return i;
      }
    }
    return count;
  }

  int64_t read(Stream * pStream, String * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!pStream->read(&pValue[i].m_data)) {
        return i;
      }
    }
    return count;
  }

  std::wstring toWide(StringView const & str) {
    return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(str.begin(), str.end());
  }

  String fromWide(std::wstring const & wstr) {
    return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(wstr).c_str();
  }
} // namespace bfc
