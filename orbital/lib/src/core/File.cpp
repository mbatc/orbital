#include "core/File.h"
#include "platform/OS.h"

namespace bfc {
  BFC_API char const* getFileMode(bfc::FileMode mode) {
    if (mode == FileMode_ReadBinary)
      return "rb";
    if (mode == FileMode_WriteBinary)
      return "wb";
    if (mode == FileMode_Read)
      return "r";
    if (mode == FileMode_Write)
      return "w";
    if (mode == FileMode_Append)
      return "a";
    if (mode == (FileMode_Append | FileMode_Read))
      return "a+";
    if (mode == (FileMode_Append | FileMode_Read | FileMode_Binary))
      return "ab+";
    if (mode == (FileMode_Read | FileMode_Write))
      return "r+";
    if (mode == (FileMode_Read | FileMode_Write | FileMode_Binary))
      return "rb+";
    return "";
  }

  FileInfo::FileInfo(FILE* pFile) {
    int descriptor = _fileno(pFile);
    if (_fstat64(descriptor, &m_stat) != 0) {
      m_exists = false;
    }
  }

  FileInfo::FileInfo(Filename const& file) {
    if (__stat64(file.c_str(), &m_stat) != 0) {
      m_exists = false;
    }
  }

  int64_t FileInfo::length() const {
    return m_stat.st_size;
  }

  Timestamp FileInfo::lastAccess() const {
    return Timestamp::fromSecs(m_stat.st_atime);
  }

  Timestamp FileInfo::lastModified() const {
    return Timestamp::fromSecs(m_stat.st_mtime);
  }

  Timestamp FileInfo::lastStatusChange() const {
    return Timestamp::fromSecs(m_stat.st_ctime);
  }

  bool FileInfo::exists() const {
    return m_exists;
  }

  bool FileInfo::isFile() const {
    return m_exists && (m_stat.st_mode & _S_IFREG) != 0;
  }

  bool FileInfo::isDirectory() const {
    return m_exists && (m_stat.st_mode & _S_IFDIR) != 0;
  }

  File::File(File && o) {
    std::swap(m_length, o.m_length);
    std::swap(m_mode, o.m_mode);
    std::swap(m_pHandle, o.m_pHandle);
    std::swap(m_streamPos, o.m_streamPos);
  }

  File::~File() {
    close();
  }

  File & File::operator=(File && o) {
    std::swap(m_length, o.m_length);
    std::swap(m_mode, o.m_mode);
    std::swap(m_pHandle, o.m_pHandle);
    std::swap(m_streamPos, o.m_streamPos);
    return *this;
  }

  bool File::open(Filename const& file, FileMode mode) {
    close();
    if (fopen_s(&m_pHandle, file.c_str(), getFileMode(mode)) != 0)
      return false;
    if (m_pHandle == nullptr)
      return false;
    m_mode  = mode;

    fseek(m_pHandle, 0, SEEK_END);
    m_length = ftell(m_pHandle);
    fseek(m_pHandle, 0, SEEK_SET);
    return true;
  }

  void File::close() {
    if (m_pHandle != nullptr) {
      fclose(m_pHandle);
    }

    m_mode = FileMode_Closed;
    m_pHandle = nullptr;
  }

  int64_t File::write(void const* data, int64_t numBytes) {
    if (m_pHandle == nullptr)
      return 0;

    int64_t bytesWritten = (int64_t)fwrite(data, 1, numBytes, m_pHandle);
    if (numBytes != bytesWritten) {
      int err = ferror(m_pHandle);
      errno   = err;
      perror("Failed to write to file");
    }

    m_streamPos += bytesWritten;
    m_length = std::max(m_length, m_streamPos);
    return bytesWritten;
  }

  int64_t File::read(void * data, int64_t numBytes) {
    if (m_pHandle == nullptr)
      return 0;

    int64_t bytesRead = (int64_t)fread(data, 1, numBytes, m_pHandle);
    m_streamPos += bytesRead;
    return bytesRead;
  }

  bool File::seek(int64_t pos, SeekOrigin origin) {
    if (m_pHandle == nullptr)
      return false;

    int64_t newPos = 0;

    switch (origin) {
    case SeekOrigin_Current:
      newPos = m_streamPos + pos;
      break;
    case SeekOrigin_End:
      newPos = m_length - pos;
      break;
    case SeekOrigin_Start:
      newPos = pos;
      break;
    default:
      return false;
    }

    newPos = std::clamp<int64_t>(newPos, 0, m_length);

    if (newPos != m_streamPos && _fseeki64(m_pHandle, newPos, SEEK_SET) != 0) {
      // Something went wrong
      return false;
    }

    m_streamPos = newPos;

    return true;
  }

  bool File::readable() const {
    return (m_mode & FileMode_Read) != 0;
  }

  bool File::writeable() const {
    return (m_mode & FileMode_Write) != 0;
  }

  bool File::seekable() const {
    return (m_mode & (FileMode_Read | FileMode_Write)) != 0;
  }

  bool File::eof() const {
    return feof(m_pHandle) != 0;
  }

  int64_t File::tell() const {
    return m_streamPos;
  }

  bool File::flush() {
    return fflush(m_pHandle) == 0;
  }

  int64_t File::length() const {
    return m_length;
  }

  FileMode File::mode() const {
    return m_mode;
  }

  bool readFile(Filename const& path, Vector<uint8_t>* pContent) {
    File f;
    if (!f.open(path, FileMode_ReadBinary)) {
      return false;
    }

    int64_t len = f.length();
    pContent->resize(len, 0);
    return f.read(pContent->data(), len) == len;
  }

  bool readTextFile(Filename const& path, String* pContent) {
    File f;
    if (!f.open(path, FileMode_Read)) {
      return false;
    }

    int64_t len = f.length();
    Vector<char> data(len + 1, 0);
    int64_t bytes = f.read(data.data(), data.size());
    data.resize(bytes + 1, 0);
    *pContent = String(std::move(data));
    return true;
  }

  bool writeFile(Filename const& path, Span<uint8_t> const& content) {
    File f;
    if (!f.open(path, FileMode_WriteBinary)) {
      return false;
    }

    return f.write(content.data(), content.size()) == content.size();
  }

  bool writeTextFile(Filename const& path, StringView const& content) {
    File f;
    if (!f.open(path, FileMode_Write)) {
      return false;
    }

    return f.write(content.data(), content.length()) == content.length();
  }

  bool deleteFile(Filename const & path) {
    return remove(path.c_str()) == 0;
  }

  bool isWritable(Filename const& path) {
    return (!fileExists(path) && os::access(path.c_str(), os::AccessFlag_Read) == 0) || File().open(path, FileMode_Append);
  }

  bool isReadable(Filename const& path) {
    return fileExists(path) && File().open(path, FileMode_Read);
  }

  bool fileExists(Filename const & path) {
    return os::access(path.c_str(), os::AccessFlag_Exists) == 0;
  }

  Filename findFile(Filename const & path, Vector<Filename> const & searchDirectories, bool * pResult) {
    if (pResult)
      *pResult = true;

    if (fileExists(path))
      return path;

    for (Filename const& dir : searchDirectories) {
      Filename testPath = dir / path;
      if (fileExists(testPath)) {
        return testPath;
      }
    }

    if (pResult)
      *pResult = false;

    return path;
  }
}
