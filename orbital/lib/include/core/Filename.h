#pragma once

#include "String.h"

namespace bfc {
  class BFC_API Filename {
    Filename(String &&path, int);
  public:
    Filename(char const *path);
    Filename(String const &path);
    Filename(StringView const &path = "");

    StringView        parent() const;
    static StringView parent(StringView const & path);

    StringView        root() const;
    static StringView root(StringView const & path);

    StringView        extension() const;
    static StringView extension(StringView const & path);

    StringView        drive() const;
    static StringView drive(StringView const & path);

    StringView        name(bool withExtension = true) const;
    static StringView name(StringView const & path, bool withExtension = true);

    StringView        path(bool withExtension) const;
    static StringView path(StringView const & path, bool withExtension);

    Filename getDirect() const;
    static Filename getDirect(StringView const & path);

    Filename concat(Filename const& filename) const;

    Filename absolute() const;

    Filename addSuffix(StringView const & suffix) const;

    Filename addPrefix(StringView const & prefix) const;

    bool isNetworkPath() const;
    bool isAbsolutePath() const;
    static bool isAbsolutePath(StringView const & path);

    int64_t length() const;

    String const& path() const;

    bool empty() const;

    StringView getView() const;

    char const* c_str() const;

    char const* data() const;
    char const* begin() const;
    char const* end() const;

    char* data();
    char* begin();
    char* end();

    bool isEquivalent(Filename const& filename) const;

    bool equals(Filename const& filename, bool ignoreCase = true) const;

    int compare(Filename const& filename, bool ignoreCase = true) const;

    bool operator==(Filename const& rhs) const;

    bool operator!=(Filename const& rhs) const;

    bool operator<(Filename const& rhs) const;

    bool operator>(Filename const& rhs) const;

    bool operator<=(Filename const& rhs) const;

    bool operator>=(Filename const& rhs) const;

    Filename operator/(Filename const& rhs) const;

    Filename& operator/=(Filename const &rhs);

    operator StringView() const;

    explicit operator String() const;

    inline friend uint64_t hash(Filename const& path) {
      return hash(path.m_path);
    }

    BFC_API friend int64_t write(Stream * pStream, Filename const * pValue, int64_t count);
    BFC_API friend int64_t read(Stream * pStream, Filename * pValue, int64_t count);

  private:
    String m_path;
  };

  inline char const * getStringFormatValue(Filename const & o) {
    return o.c_str();
  }
}
