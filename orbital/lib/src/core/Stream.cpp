#include "core/Stream.h"
#include "core/File.h"
#include "core/URI.h"

namespace bfc {
  int64_t Stream::write(void const * data, int64_t length) {
    BFC_UNUSED(data, length);
    return 0;
  }

  int64_t Stream::read(void * data, int64_t length) {
    BFC_UNUSED(data, length);
    return 0;
  }

  bool Stream::seek(int64_t pos, SeekOrigin origin) {
    BFC_UNUSED(pos, origin);
    return false;
  }

  bool Stream::flush() {
    return false;
  }

  MemoryStream::MemoryStream() {}

  MemoryStream::MemoryStream(Vector<uint8_t> data, SeekOrigin origin)
    : m_data(std::move(data)) 
    , m_streamPos(origin == SeekOrigin_End ? m_data.size() : 0) {}

  bool MemoryStream::readable() const {
    return true;
  }

  bool MemoryStream::writeable() const {
    return true;
  }

  bool MemoryStream::seekable() const {
    return true;
  }

  bool MemoryStream::eof() const {
    return m_streamPos == m_data.size();
  }

  int64_t MemoryStream::write(void const * data, int64_t length) {
    int64_t newLen = std::max(m_streamPos + length, m_data.size());
    m_data.resize(newLen);
    memcpy(m_data.data() + m_streamPos, data, length);
    m_streamPos += length;
    return length;
  }

  int64_t MemoryStream::read(void * data, int64_t length) {
    int64_t bytesRead = std::min(m_data.size() - m_streamPos, length);
    memcpy(data, m_data.data() + m_streamPos, bytesRead);
    m_streamPos += bytesRead;
    return bytesRead;
  }

  const Vector<uint8_t> & MemoryStream::storage() const {
    return m_data;
  }

  void MemoryStream::clear() {
    m_data.clear();
    m_streamPos = 0;
  }

  int64_t MemoryStream::tell() const {
    return m_streamPos;
  }

  bool MemoryStream::seek(int64_t pos, SeekOrigin origin) {
    int64_t newPos = 0;

    switch (origin) {
    case SeekOrigin_Current: newPos = m_streamPos + pos; break;
    case SeekOrigin_End: newPos = m_data.size() - pos; break;
    case SeekOrigin_Start: newPos = pos; break;
    default: return false;
    }

    m_streamPos = std::clamp<int64_t>(newPos, 0, m_data.size());

    return m_streamPos == newPos;
  }

  MemoryReader::MemoryReader(const Span<uint8_t> & data)
    : m_data(data) {}

  bool MemoryReader::readable() const {
    return true;
  }

  bool MemoryReader::writeable() const {
    return false;
  }

  bool MemoryReader::seekable() const {
    return true;
  }

  bool MemoryReader::eof() const {
    return m_streamPos == m_data.size();
  }

  bool MemoryReader::seek(int64_t pos, SeekOrigin origin) {
    int64_t newPos = 0;
    switch (origin) {
    case SeekOrigin_Current: newPos = m_streamPos + pos; break;
    case SeekOrigin_End: newPos = m_data.size() - pos; break;
    case SeekOrigin_Start: newPos = pos; break;
    default: return false;
    }

    m_streamPos = std::clamp<int64_t>(newPos, 0, m_data.size());

    return m_streamPos == newPos;
  }

  int64_t MemoryReader::tell() const {
    return m_streamPos;
  }

  int64_t MemoryReader::read(void * data, int64_t length) {
    int64_t bytesRead = std::min(m_data.size() - m_streamPos, length);
    memcpy(data, m_data.data() + m_streamPos, bytesRead);
    m_streamPos += bytesRead;
    return bytesRead;
  }

  TextReader::TextReader(Stream * pStream)
    : m_pStream(pStream) {}

  bool TextReader::eof() const {
    return m_pStream->eof();
  }

  StringView TextReader::readLine() {
    m_buffer.clear();
    char c = 0;
    while (!m_pStream->eof()) {
      if (m_pStream->read(&c, 1) != 0) {
        if (c == '\n') {
          break;
        }

        m_buffer.pushBack(c);
      }
    }

    return last();
  }

  StringView TextReader::read(int64_t length) {
    m_buffer.clear();
    m_buffer.resize(length);
    m_buffer.resize(m_pStream->read(m_buffer.data(), length));
    return last();
  }

  StringView TextReader::last() const {
    return StringView(m_buffer.begin(), m_buffer.end());
  }

  TextWriter::TextWriter(Stream * pStream)
    : m_pStream(pStream) {}

  bool TextWriter::writeLine(StringView const & line) {
    return write(line) && write("\n");
  }

  bool TextWriter::write(StringView const & line) {
    return m_pStream->write(line.begin(), line.length()) == line.length();
  }

  int64_t read(Stream * pStream, void * pValue, int64_t size) {
    return pStream->read(pValue, size);
  }

  int64_t write(Stream * pStream, void const * value, int64_t size) {
    return pStream->write(value, size);
  }

  Ref<Stream> openURI(URI const & uri, FileMode mode) {
    if (uri.empty()) {
      return nullptr;
    }

    if (uri.scheme().empty() || uri.scheme() == "file") {
      Ref<File> pFile = NewRef<File>();
      if (pFile->open(uri.path(), mode)) {
        return pFile;
      }
    } else {
      BFC_ASSERT(false, "URI scheme (%*.s) is not supported", uri.scheme().length(), uri.scheme().data());
    }

    return nullptr;
  }

  bool uriExists(URI const & uri) {
    if (uri.scheme().empty() || uri.scheme() == "file") {
      return fileExists(uri.path().c_str());
    } else {
      return openURI(uri, FileMode_Read) != nullptr;
    }
  }

  BFC_API bool readUntilEof(Stream * pStream, Vector<uint8_t> *pContent) {
    if (pStream == nullptr) {
      return false;
    }

    if (pStream->length() != -1) {
      int64_t len = pStream->length();
      pContent->resize(len, 0);
      int64_t bytes = pStream->read(pContent->data(), len);
      pContent->resize(bytes);
      return true;
    }

    const int64_t readSize  = 32ll * 1024 * 1024;
    int64_t       totalRead = 0;
    pContent->resize(readSize, 0);
    while (int64_t bytes = pStream->read(pContent->begin() + totalRead, readSize)) {
      pContent->resize(totalRead + readSize, 0);
      totalRead += bytes;
    }
    pContent->resize(totalRead);
    return pContent->size();
  }

  BFC_API bool readFile(URI const & uri, Vector<uint8_t> * pContent) {
    return readUntilEof(openURI(uri, FileMode_ReadBinary).get(), pContent);
  }

  BFC_API bool readTextURI(URI const & uri, String * pContent) {
    Vector<uint8_t> data;
    if (!readUntilEof(openURI(uri, FileMode_Read).get(), &data))
      return false;
    if (data.back() != 0)
      data.pushBack(0);
    *pContent = String(std::move((Vector<char>&)data));
    return true;
  }

  BFC_API bool writeURI(URI const & uri, Span<uint8_t> const & content) {
    Ref<Stream> pStream = openURI(uri, FileMode_WriteBinary);
    return pStream != nullptr && pStream->write(content.data(), content.size()) == content.size();
  }

  BFC_API bool writeTextURI(URI const & uri, StringView const & content) {
    Ref<Stream> pStream = openURI(uri, FileMode_Write);
    return pStream != nullptr && pStream->write(content.data(), content.length()) == content.length();
  }
} // namespace bfc
