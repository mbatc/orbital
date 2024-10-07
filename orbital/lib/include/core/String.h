#pragma once

#include "Array.h"
#include "Vector.h"
#include "StringView.h"

#include <locale>
#include <cstdio>

namespace bfc {
  /// Returns the element to pass to the underlying format function in String::format.
  /// Overload this function to change what is returned for a specific type.
  template<typename T>
  T const & getStringFormatValue(T const & o) {
    return o;
  };

  class BFC_API String {
  public:
    String();

    String(char const* str);

    String(char const* first, char const* last);

    String(char const* str, int64_t length);

    String(Span<char> const& data);

    String(Vector<char> && data);

    StringView substr(int64_t start, int64_t count = npos) const;

    StringView getView() const;

    int64_t length() const;

    bool empty() const;

    bool equals(StringView const& view, bool ignoreCase = false) const;

    int compare(StringView const& with, bool ignoreCase = false) const;

    String concat(StringView const& rhs) const;

    String replace(StringView const& match, StringView const& replace, int64_t start = 0) const;

    /// Return a new string with a section of it replaced with a new string.
    /// @param section A view into the part of the string to be replaced.
    /// @param replace The string to replace the section with.
    String replacePart(StringView const & section, StringView const & replace) const;

    /// Parse the string for escape character codes and replace them with the corresponding
    /// ASCII character sequence.
    String escaped() const;

    /// Parse the string for escape character sequences and replace them with the actual
    /// escape character code.
    String unescaped() const;

    StringView trim(StringView const& chars = " \r\n\t\v") const;

    String toLower() const;
    String toUpper() const;

    int64_t find(char needle, int64_t start = 0) const;
    int64_t find(StringView const& needle, int64_t start = 0) const;

    int64_t findLast(char needle, int64_t start = npos) const;
    int64_t findLast(StringView const& needle, int64_t start = npos) const;

    int64_t findFirstOf(StringView const& needle, int64_t start = 0) const;
    int64_t findLastOf(StringView const& needle, int64_t start = npos) const;

    int64_t findFirstNotOf(char needle, int64_t start = 0) const;
    int64_t findFirstNotOf(StringView const& needle, int64_t start = 0) const;

    int64_t findLastNotOf(char needle, int64_t start = npos) const;
    int64_t findLastNotOf(StringView const& needle, int64_t start = npos) const;

    bool startsWith(StringView const& match) const;
    bool endsWith(StringView const& match) const;

    Vector<StringView> split(Span<StringView> const& sep, bool dropEmpty) const;
    Vector<StringView> split(StringView const & sep, bool dropEmpty) const;

    Vector<StringView> split(Span<StringView> const & sep, SplitFlags flags) const;
    Vector<StringView> split(StringView const & sep, SplitFlags flags) const;

    static String join(Span<String> const& strings, StringView const& sep, bool ignoreEmpty = false);

    static String join(Span<StringView> const& strings, StringView const& sep, bool ignoreEmpty = false);

    template<typename... Args>
    static String format(char const * fmt, Args const &... args) {
      int length = std::snprintf(nullptr, 0, fmt, getStringFormatValue(args)...);

      String str;
      str.resize(length, ' ');
      std::snprintf(str.data(), str.length() + 1, fmt, getStringFormatValue(args)...);

      return str;
    }

    int64_t capacity() const;

    bool reserve(int64_t newCapacity);

    void resize(int64_t newLength, char c);

    void insert(int64_t index, char c);

    void insert(int64_t index, char const* data, int64_t count);

    void insert(int64_t index, StringView const& data);

    void pushBack(char c);

    void pushBack(char const* data, int64_t count);

    void pushBack(StringView const& data);

    void erase(int64_t index, int64_t count);

    void pushFront(char c);

    void pushFront(char const* data, int64_t count);

    void pushFront(StringView const& data);

    char popBack();
    char popFront();

    char* c_str();
    char const* c_str() const;

    char* begin();
    char* end();
    char* data();

    char const* begin() const;
    char const* end() const;
    char const* data() const;

    operator StringView() const;

    char& operator[](int64_t index);

    char const& operator[](int64_t index) const;

    bool operator==(StringView const& rhs) const;

    bool operator!=(StringView const& rhs) const;

    bool operator<(StringView const& rhs) const;

    bool operator>(StringView const& rhs) const;

    bool operator<=(StringView const& rhs) const;

    bool operator>=(StringView const& rhs) const;

    String operator+(StringView const & rhs) const;
    String operator+(String const & rhs) const;
    String operator+(char const * rhs) const;

    friend BFC_API int64_t write(Stream * pStream, String const * pValue, int64_t count);
    friend BFC_API int64_t read(Stream * pStream, String * pValue, int64_t count);

  private:
    Vector<char> m_data;
  };

  BFC_API String operator+(StringView const & lhs, String const & rhs);
  BFC_API String operator+(char const * lhs, String const & rhs);
  BFC_API String operator+(StringView const & lhs, StringView const & rhs);
  BFC_API String operator+(StringView const & lhs, char const * rhs);

  BFC_API String toString(double value);
  BFC_API String toString(int64_t value);

  BFC_API int64_t write(Stream * pStream, String const * pValue, int64_t count);
  BFC_API int64_t read(Stream * pStream, String * pValue, int64_t count);

  BFC_API std::wstring toWide(StringView const & str);
  BFC_API String fromWide(std::wstring const & wstr);

  template<typename T, std::enable_if_t<std::is_integral_v<T>>* = 0>
  constexpr auto toHex(T const & value) {
    Array<char, sizeof(T) * 2> ret;
    T                          remaining = value;
    for (int64_t i = ret.size() - 1; i >= 0; --i) {
      uint8_t nibble = remaining & 0xF;
      ret[i]         = (nibble > 9 ? ('A' - 10) : '0') + nibble;
      remaining >>= 4;
    }

    return ret;
  }

  template<typename T, std::enable_if_t<std::is_integral_v<T>>* = 0>
  constexpr T fromHex(StringView const & hex) {
    T ret = 0;
    for (int64_t i = 0; i < hex.length(); ++i) {
      uint8_t digit  = hex[i];
      uint8_t nibble = digit - (digit > '9' ? (islower(digit) ? 'a' : 'A') - 10 : '0');
      ret            = (ret << 4) + nibble;
    }

    return ret;
  }

  constexpr bool isHex(char c) {
    if (c >= '0' && c <= '9')
      return true;
    if (islower(c))
      return c >= 'a' && c <= 'f';
    else
      return c >= 'a' && c <= 'f';
  }

  inline char const* getStringFormatValue(String const& o) {
    return o.c_str();
  }
}

namespace std {
  template<>
  struct hash<bfc::String> {
    size_t operator()(const bfc::String& o) const {
      return std::hash<std::string_view>{}(std::string_view(o.begin(), o.length()));
    }
  };
}
