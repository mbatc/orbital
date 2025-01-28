#pragma once

#include "String.h"
#include "Filename.h"

namespace bfc {
  class BFC_API URI {
  public:
    static URI File(Filename const & path);
    
    struct Components {
      Components() = default;
      Components(URI const & uri)
        : scheme(uri.scheme())
        , auth(uri.authority())
        , path(uri.pathView())
        , fragment(uri.fragment())
        , query(uri.query()) {}

      StringView scheme;
      StringView auth;
      StringView path;
      StringView fragment;
      StringView query;

      String toString() const;
    };

    URI() = default;
    URI(String const & uri);
    URI(char const * uri);
    URI(StringView const & uri);
    URI(StringView const & scheme, StringView const & host, StringView const & port, StringView const & user, StringView const & path, StringView const & query,
        StringView const & fragment);

    String const& str() const;

    StringView scheme() const;
    static StringView scheme(StringView const & uri);

    StringView authority() const;
    static StringView authority(StringView const & uri);

    StringView host() const;
    static StringView host(StringView const & uri);

    StringView port() const;
    static StringView port(StringView const & uri);

    StringView user() const;
    static StringView user(StringView const & uri);

    StringView query() const;
    static StringView query(StringView const & uri);

    StringView fragment() const;
    static StringView fragment(StringView const & uri);

    StringView pathView() const;
    static StringView path(StringView const & uri);

    Filename path() const;

    char * begin();
    char * end();

    char const * begin() const;
    char const * end() const;

    char const * c_str() const;

    bool operator==(URI const & rhs) const;
    bool operator!=(URI const & rhs) const;

    /// Get a copy of the URI with the scheme changed.
    URI withScheme(StringView const & newScheme) const;

    /// Get a copy of the URI with the authority changed.
    URI withAuthority(StringView const & newAuth) const;

    /// Get a copy of the URI with the port changed.
    URI withPort(StringView const & newPort) const;

    /// Get a copy of the URI with the host changed.
    URI withHost(StringView const & newHost) const;

    /// Get a copy of the URI with the user changed.
    URI withUser(StringView const & newUser) const;

    /// Get a copy of the URI with the query changed.
    URI withQuery(StringView const & newQuery) const;

    /// Get a copy of the URI with the fragment changed.
    URI withFragment(StringView const & newFragment) const;

    /// Get a copy of the URI with the path changed.
    URI withPath(StringView const & newPath) const;

    /// Resolve a relative URI reference.
    URI resolveRelativeReference(URI const & reference, bool strict = false) const;

  private:
    URI replacePartPrefixed(StringView const & section, StringView const & replace, StringView const & prefixOnEmpty) const;
    URI replacePartSuffixed(StringView const & section, StringView const & replace, StringView const & suffixOnEmpty) const;

    String m_uri;
  };

  inline char const * getStringFormatValue(URI const & o) {
    return o.c_str();
  }

  BFC_API Vector<URI> walk(URI const & uri, bool recursive);
} // namespace bfc

namespace std {
  template<>
  struct hash<bfc::URI> {
    size_t operator()(const bfc::URI & o) const {
      return std::hash<bfc::String>{}(o.str());
    }
  };
} // namespace std
