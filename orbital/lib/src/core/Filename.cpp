#include "core/Filename.h"
#include "core/Stream.h"
#include "platform/OS.h"

namespace bfc {
  Filename::Filename(String&& path, int)
      : m_path(std::move(path)) {}

  Filename::Filename(char const * path)
    : Filename(StringView(path)) {}

  Filename::Filename(String const &path)
    : Filename(path.getView()) {}

  Filename::Filename(StringView const& path)
      : m_path(path) {
    m_path = m_path.replace("\\", "/", m_path.startsWith("\\\\") ? 2 : 0);
    int64_t lastLength;
    do {
      lastLength = m_path.length();
      m_path = m_path.replace("//", "/");
    } while (m_path.length() != lastLength);
  }

  StringView Filename::parent() const {
    return parent(m_path);
  }

  StringView Filename::root() const {
    return root(m_path);
  }

  StringView Filename::name(bool withExtension) const {
    return name(m_path, withExtension);
  }

  StringView Filename::extension() const {
    return extension(m_path);
  }

  StringView Filename::path(bool withExtension) const {
    return path(m_path, withExtension);
  }

  StringView Filename::drive() const {
    return drive(m_path);
  }

  StringView Filename::parent(StringView const & path) {
    return path.substr(0, path.length() - name(path).length() - 1);
  }

  StringView Filename::root(StringView const & path) {
    StringView driveLetter = drive(path);
    if (driveLetter.length() > 0)
      return driveLetter;
    int64_t start      = path[0] == '/' ? 1 : 0;
    int64_t firstSlash = path.find('/', start);
    return firstSlash == npos ? StringView() : path.substr(start, firstSlash - 1);
  }

  StringView Filename::extension(StringView const & path) {
    StringView stub = name(path);
    int64_t    dot  = stub.findLast('.');
    if (dot == npos)
      return StringView();
    return stub.substr(dot + 1);
  }

  StringView Filename::drive(StringView const & path) {
    int64_t drive = path.find(':');
    return drive == npos ? StringView() : path.substr(0, drive);
  }

  StringView Filename::name(StringView const & path, bool withExtension) {
    int64_t    slash = path.findLast('/');
    StringView nm    = slash == npos ? path : path.substr(slash + 1);
    if (withExtension) {
      return nm;
    }

    StringView ext = extension(path);
    if (ext.size() > 0) {
      return StringView(nm.begin(), ext.begin() - 1);
    }

    return nm;
  }

  StringView Filename::path(StringView const & path, bool withExtension) {
    StringView fullPath = path;
    if (withExtension) {
      return fullPath;
    }

    StringView ext = extension(path);
    if (ext.size() > 0) {
      return StringView(fullPath.begin(), ext.begin() - 1);
    }

    return fullPath;
  }

  static StringView walkUp(StringView const& path, int64_t start, int64_t *pStepUpRemaining) {
    int64_t stepUp = 0;
    char* last = 0;
    char* first = last;
    for (int64_t i = start; i >= 0;) {
      start = path.findLast('/', i);
      StringView part;
      if (start == npos)
        part = path.substr(0, i + 1);
      else
        part = path.substr(start, i - start + 1);

      if (part != "/." || part == ".") // Skip this
      {
        if (part == "/.." || part == "..") { // Step up
          ++stepUp;
        }
        else {
          if (stepUp == 0) {
            return part;
          }
          else {
            --stepUp; // Skip this directory
          }
        }
      }

      i -= part.length();
    }

    *pStepUpRemaining = stepUp;

    return StringView(path.begin(), 0ll);
  }

  Filename Filename::getDirect() const {
    return getDirect(path());
  }

  Filename Filename::getDirect(StringView const & path) {
    Vector<char> ret;
    ret.reserve(path.length() + 1);
    ret.resize(ret.capacity(), 0);

    int64_t stepUp       = 0;
    int64_t directLength = 0;

    char * last  = ret.data() + ret.capacity();
    char * first = last - 1;

    int64_t stepUps = 0;
    for (int64_t offset = path.length(); offset >= 0;) {
      StringView part = walkUp(path, offset, &stepUps);
      for (int64_t i = part.length() - 1; i >= 0; --i) {
        *(--first) = part[i];
      }
      offset = part.begin() - path.begin() - 1;
    }

    if (stepUps > 0) {
      ++first;
      while (stepUps-- > 0) {
        first -= 3;
        memcpy(first, "../", 3);
      }
    }

    int64_t len = last - first;
    mem::copyAssign(ret.data(), first, len);
    ret.resize(len);
    return Filename(std::move(ret), 0);
  }

  Filename Filename::concat(Filename const& filename) const {
    if (filename.drive().length() > 0)
      return filename;
    else if (length() > 0)
      return Filename(String::format("%s/%s", m_path, filename.m_path)).getDirect();
    else
      return filename.getDirect();
  }

  bool Filename::isNetworkPath() const {
    return m_path.startsWith("\\\\");
  }

  bool Filename::isAbsolutePath() const {
    return isAbsolutePath(m_path);
  }

  bool Filename::isAbsolutePath(StringView const & path) {
    return path.startsWith("/") || Filename::drive(path).length() > 0;
  }

  int64_t Filename::length() const {
    return m_path.length();
  }

  String const& Filename::path() const {
    return m_path;
  }

  bool Filename::empty() const {
    return length() == 0;
  }

  StringView Filename::getView() const {
    return m_path.getView();
  }

  char const* Filename::c_str() const {
    return m_path.c_str();
  }

  char const* Filename::data() const {
    return m_path.data();
  }

  char const* Filename::begin() const {
    return m_path.begin();
  }

  char const* Filename::end() const {
    return m_path.end();
  }

  char* Filename::data() {
    return m_path.data();
  }

  char* Filename::begin() {
    return m_path.begin();
  }

  char* Filename::end() {
    return m_path.end();
  }

  bool Filename::isEquivalent(Filename const& other) const {
    int64_t myOffset    = length() - 1;
    int64_t otherOffset = other.length() - 1;
    int64_t aStepUps = 0;
    int64_t bStepUps = 0;
    while (true) {
      StringView a = walkUp(path(), myOffset, &aStepUps);
      StringView b = walkUp(other.path(), otherOffset, &bStepUps);
      if (!a.compare(b, true))
        return false;

      if (a.length() == 0)
        break;
    }

    return aStepUps == bStepUps;
  }

  Filename Filename::absolute() const {
    return os::getAbsolutePath(*this);
  }

  Filename Filename::addPrefix(StringView const & prefix) const {
    StringView base = parent();
    StringView nm   = name();
    if (base.length() > 0)
      return String::format("%.*s/%.*s.%.*s", base.length(), base.data(), prefix.length(), prefix.data(), nm.length(), nm.data());
    else
      return String::format("%.*s.%.*s", prefix.length(), prefix.data(), nm.length(), nm.data());
  }

  Filename Filename::addSuffix(StringView const & suffix) const {
    StringView base = path(false);
    StringView ext  = extension();
    return String::format("%.*s%.*s.%.*s", base.length(), base.data(), suffix.length(), suffix.data(), ext.length(), ext.data());
  }

  bool Filename::equals(Filename const& filename, bool ignoreCase) const {
    return path().equals(filename.path(), ignoreCase);
  }

  int Filename::compare(Filename const& filename, bool ignoreCase) const {
    return path().compare(filename.path(), ignoreCase);
  }

  bool Filename::operator==(Filename const& rhs) const {
    return equals(rhs);
  }

  bool Filename::operator!=(Filename const& rhs) const {
    return !equals(rhs);
  }

  bool Filename::operator<(Filename const& rhs) const {
    return compare(rhs) < 0;
  }

  bool Filename::operator>(Filename const& rhs) const {
    return compare(rhs) > 0;
  }

  bool Filename::operator<=(Filename const& rhs) const {
    return !operator>(rhs);
  }

  bool Filename::operator>=(Filename const& rhs) const {
    return !operator<(rhs);
  }

  Filename Filename::operator/(Filename const& rhs) const {
    return concat(rhs);
  }

  Filename& Filename::operator/=(Filename const& rhs) {
    return *this = concat(rhs);
  }

  Filename::operator StringView() const {
    return getView();
  }

  Filename::operator String() const {
    return path();
  }

  int64_t write(Stream * pStream, Filename const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!pStream->write(pValue[i].m_path))
        return i;
    }
    return count;
  }

  int64_t read(Stream * pStream, Filename * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!pStream->read(&pValue[i].m_path))
        return i;
    }
    return count;
  }
}
