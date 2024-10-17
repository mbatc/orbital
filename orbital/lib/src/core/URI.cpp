#include "core/URI.h"
#include "core/File.h"

namespace bfc {

  static bool isReserved(char c) {
    static StringView const genericDelimiters = ":/?#[]@";
    static StringView const subDelimiters     = "!$&'()*+,;=";
    return genericDelimiters.find(c) != npos || subDelimiters.find(c) != npos;
  }

  static bool isDigit(char c) {
    return c >= '0' && c <= '9';
  }

  static bool isAlpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
  }

  static bool isUnreserved(char c) {
    static StringView const unreserved = "-._~";
    return isAlpha(c) || isDigit(c) || unreserved.find(c) != npos;
  }

  URI URI::File(Filename const & path) {
    return String("file:///").concat(path);
  }

  URI::URI(String const & uri)
    : URI(StringView(uri)) {}

  URI::URI(char const * uri)
    : URI(StringView(uri)) {}

  URI::URI(StringView const & uri)
    : URI(scheme(uri), host(uri), port(uri), user(uri), Filename(path(uri)), query(uri), fragment(uri)) {
  }

  URI::URI(StringView const & scheme, StringView const & host, StringView const & port, StringView const & user, StringView const & path, StringView const & query, StringView const & fragment) {
    if (scheme.empty() && host.empty() && port.empty() && user.empty() && path.empty() && query.empty() && fragment.empty()) {
      return;
    }

    String auth;
    if (user.length() > 0)
      auth = user + "@";
    if (host.length() > 0)
      auth = auth + host;
    if (port.length() > 0)
      auth = auth + ":" + port;

    BFC_ASSERT(auth.length() == 0 || host.length() > 0, "host cannot be empty if 'user' or 'port' are specified.");

    Components uri;
    uri.auth     = auth;
    uri.scheme   = scheme;
    uri.path     = path;
    uri.query    = query;
    uri.fragment = fragment;
    m_uri        = uri.toString();
  }

  String const & URI::str() const {
    return m_uri;
  }

  StringView URI::scheme() const {
    return scheme(m_uri);
  }

  StringView URI::authority() const {
    return authority(m_uri);
  }

  StringView URI::host() const {
    return host(m_uri);
  }

  StringView URI::port() const {
    return port(m_uri);
  }

  StringView URI::user() const {
    return user(m_uri);
  }

  StringView URI::query() const {
    return query(m_uri);
  }

  StringView URI::fragment() const {
    return fragment(m_uri);
  }

  StringView URI::pathView() const {
    return path(m_uri);
  }

  StringView URI::scheme(StringView const & uri) {
    int64_t schemeEnd  = uri.find(":");
    int64_t authStart  = uri.find("//");
    int64_t firstSlash = uri.find("/");

    if (schemeEnd == npos || schemeEnd > authStart || schemeEnd > firstSlash) {
      return StringView();
    }

    return uri.substr(0, schemeEnd);
  }

  StringView URI::authority(StringView const & uri){
    int64_t authStart = uri.find("//");
    if (authStart == npos || authStart == 0 || uri[authStart - 1] != ':') {
      return StringView();
    }

    authStart += 2;

    int64_t pathStart     = uri.find('/', authStart);
    int64_t queryStart    = uri.find('?', authStart);
    int64_t fragmentStart = uri.find('#', authStart);
    int64_t authEnd       = std::min(fragmentStart, std::min(pathStart, queryStart));

    return uri.substr(authStart, authEnd - authStart);
  }

  StringView URI::host(StringView const & uri) {
    StringView auth      = authority(uri);
    int64_t    hostStart = auth.find('@');
    int64_t    hostEnd   = auth.find(':');

    if (hostStart == npos)
      hostStart = 0;

    return auth.substr(hostStart, hostEnd - hostStart).substr(1);
  }

  StringView URI::port(StringView const & uri){
    StringView auth      = authority(uri);
    int64_t    portStart = auth.find(':');

    return auth.substr(portStart).substr(1);
  }

  StringView URI::user(StringView const & uri) {
    StringView auth    = authority(uri);
    int64_t    userEnd = auth.find('@');
    if (userEnd == npos) {
      return StringView();
    }

    return auth.substr(0, userEnd);
  }

  StringView URI::query(StringView const & uri) {
    int64_t queryStart    = uri.findLast('?');
    int64_t fragmentStart = uri.find('#', queryStart);
    return uri.substr(queryStart, fragmentStart - queryStart).substr(1);
  }

  StringView URI::fragment(StringView const & uri) {
    return uri.substr(uri.findLast('#')).substr(1);
  }

  StringView URI::path(StringView const & uri) {
    StringView auth = authority(uri);
    StringView schm = scheme(uri);

    int64_t pathStart = npos;
    if (auth.length())
      pathStart = uri.find('/', auth.end() - uri.begin());
    else if (schm.length())
      pathStart = uri.find(':', schm.end() - uri.begin());

    if (pathStart == npos)
      pathStart = 0;
    else
      pathStart += 1; // Skip character we found

    int64_t pathEnd = uri.findFirstOf("#?", pathStart);

    bfc::StringView parsed = uri.substr(pathStart, pathEnd - pathStart);
    return parsed;
  }

  Filename URI::path() const {
    return pathView();
  }

  char * URI::begin() {
    return m_uri.begin();
  }

  char * URI::end() {
    return m_uri.begin();
  }

  char const * URI::begin() const {
    return m_uri.begin();
  }

  char const * URI::end() const {
    return m_uri.begin();
  }

  char const * URI::c_str() const {
    return m_uri.c_str();
  }

  bool URI::operator==(URI const & rhs) const {
    return m_uri == rhs.m_uri;
  }

  bool URI::operator!=(URI const & rhs) const {
    return m_uri != rhs.m_uri;
  }

  URI URI::withScheme(StringView const & newScheme) const {
    return replacePartSuffixed(scheme(), newScheme, ":");
  }

  URI URI::withAuthority(StringView const & newAuth) const {
    return replacePartPrefixed(authority(), newAuth, "//");
  }

  URI URI::withPort(StringView const & newPort) const {
    if (host().empty()) {
      return withHost("localhost").withPort(newPort);
    }

    return replacePartPrefixed(port(), newPort, ":");
  }

  URI URI::withHost(StringView const & newHost) const {
    return replacePartPrefixed(host(), newHost, "@");
  }

  URI URI::withUser(StringView const & newUser) const {
    if (host().empty()) {
      return withHost("localhost").withUser(newUser);
    }

    return replacePartSuffixed(user(), newUser, "@");
  }

  URI URI::withQuery(StringView const & newQuery) const {
    return replacePartPrefixed(query(), newQuery, "?");
  }

  URI URI::withFragment(StringView const & newFragment) const {
    return replacePartPrefixed(fragment(), newFragment, "#");
  }

  URI URI::withPath(StringView const & newPath) const {
    return m_uri.replacePart(pathView(), newPath);
  }

  URI URI::resolveRelativeReference(URI const & reference, bool strict) const {
    Components base = *this;
    Components ref  = reference;
    Components transformed;
    Filename   newPath;

    if (!strict || base.scheme == ref.scheme) {
      ref.scheme = "";
    }

    if (ref.scheme.empty()) {
      if (ref.auth.empty()) {
        if (ref.path == "") {
          transformed.path = base.path;

          if (ref.query.empty()) {
            transformed.query = base.query;
          } else {
            transformed.query = ref.query;
          }
        } else {
          if (ref.path.startsWith("/")) {
            newPath = Filename::getDirect(ref.path);
          } else {
            newPath = Filename(base.path).concat(ref.path);
          }

          transformed.query = ref.query;
        }
        transformed.auth = base.auth;
      } else {
        transformed.auth  = ref.auth;
        transformed.query = ref.query;

        newPath = ref.path;
      }

      transformed.scheme = base.scheme;
    } else {
      transformed.scheme = ref.scheme;
      transformed.auth   = ref.auth;
      transformed.query  = ref.query;

      newPath = ref.path;
    }

    transformed.fragment = ref.fragment;
    transformed.path     = newPath;
    return transformed.toString();
  }
  
  URI URI::replacePartPrefixed(StringView const & section, StringView const & replace, StringView const & prefixOnEmpty) const {
    if (section.empty()) {
      return m_uri.replacePart(section, prefixOnEmpty + replace);
    } else {
      return m_uri.replacePart(section, replace);
    }
  }

  URI URI::replacePartSuffixed(StringView const & section, StringView const & replace, StringView const & suffixOnEmpty) const {
    if (section.empty()) {
      return m_uri.replacePart(section, replace + suffixOnEmpty);
    } else {
      return m_uri.replacePart(section, replace);
    }
  }

  String URI::Components::toString() const {
    if (scheme.empty() && auth.empty() && path.empty() && query.empty() && fragment.empty()) {
      return "";
    }

    String  ret;
    int64_t worstCaseSize = scheme.size() + auth.size() + path.size() + query.size() + fragment.size() + 10;
    ret.reserve(worstCaseSize);

    if (scheme.length() > 0) {
      ret = scheme + ":";
    }

    if (auth.length() > 0) {
      ret = ret + "//" + auth;
    }

    if (Filename::isAbsolutePath(path)) {
      if (!auth.empty()) {
        ret = ret + "/";
      }

      if (path.startsWith("/")) {
        // On windows strip the leading '/' for an absolute path as we only need the drive letter.
        ret = ret + path.substr(1);
      } else {
        ret = ret + path;
      }
    } else {
      if (!auth.empty()) {
        ret = ret + "/";
      }

      if (scheme.empty() && path.find("/") > path.find(":")) {
        ret = ret + "./";
      }

      ret = ret + path;
    }

    if (query.length() > 0)
      ret = ret + "?" + query;

    if (fragment.length() > 0)
      ret = ret + "#" + fragment;

    return ret;
  }
} // namespace bfc