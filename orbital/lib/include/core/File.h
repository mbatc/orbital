#pragma once

#include "Stream.h"
#include "Filename.h"
#include "Timestamp.h"

#include <optional>

namespace bfc {
  BFC_API char const* getFileMode(bfc::FileMode mode);

  class BFC_API FileInfo {
  public:
    FileInfo() = default;

    FileInfo(FILE* pFile);

    FileInfo(Filename const& file);

    int64_t length() const;

    Timestamp lastAccess() const;

    Timestamp lastModified() const;

    Timestamp lastStatusChange() const;

    bool exists() const;

    bool isFile() const;

    bool isDirectory() const;

  private:
    bool m_exists = true;
    struct _stat64 m_stat = { 0 };
  };

  class BFC_API File : public Stream {
  public:
    using Stream::write;
    using Stream::read;

    File()               = default;
    File(File const & o) = delete;
    File(File && o);
    File & operator=(File && o);
    ~File();

    bool open(Filename const& file, FileMode mode = FileMode(FileMode_Read | FileMode_Write | FileMode_Binary));

    void close();

    virtual int64_t write(void const* data, int64_t length) override;

    virtual int64_t read(void* data, int64_t length) override;

    virtual bool seek(int64_t pos, SeekOrigin origin = SeekOrigin_Current) override;

    virtual bool readable() const override;

    virtual bool writeable() const override;

    virtual bool seekable() const override;

    virtual bool eof() const override;

    virtual int64_t tell() const override;

    virtual bool flush() override;

    virtual int64_t length() const override;

    FileMode mode() const;

  private:
    FILE* m_pHandle = 0;
    FileMode m_mode = FileMode_Closed;
    int64_t m_streamPos = 0;
    int64_t m_length = 0;
  };

  BFC_API bool readFile(Filename const& path, Vector<uint8_t> *pContent);

  BFC_API bool readTextFile(Filename const& path, String *pContent);

  BFC_API bool writeFile(Filename const& path, Span<uint8_t> const & content);

  BFC_API bool writeTextFile(Filename const& path, StringView const & content);

  BFC_API bool fileExists(Filename const& path);

  BFC_API bool deleteFile(Filename const & path);

  BFC_API bool isWritable(Filename const & path);

  BFC_API bool isReadable(Filename const & path);

  BFC_API Filename findFile(Filename const & path, Vector<Filename> const & searchDirectories, bool *pResult = nullptr);

  template<typename T>
  bool writeFile(Filename const & path, T const & o) {
    File f;
    if (!f.open(path, FileMode_WriteBinary)) {
      return false;
    }
    return f.write(o);
  }

  template<typename T>
  std::optional<T> readFile(Filename const & path) {
    File f;
    if (!f.open(path, FileMode_ReadBinary)) {
      return {};
    }

    Uninitialized<T> buffer;
    if (f.read(buffer.ptr()) != 1) {
      return {};
    }

    return std::move(buffer.get());
  }
}
