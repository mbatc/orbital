#pragma once

#include "Core.h"
#include "StringView.h"

#include <optional>

namespace bfc {
  class URI;
  class Stream;
  enum FileMode;

  enum FileMode {
    FileMode_Closed      = 0,
    FileMode_Read        = 1 << 0,
    FileMode_Write       = 1 << 1,
    FileMode_Append      = 1 << 2,
    FileMode_Binary      = 1 << 3,
    FileMode_ReadBinary  = FileMode_Read | FileMode_Binary,
    FileMode_WriteBinary = FileMode_Write | FileMode_Binary,
  };

  enum SeekOrigin {
    SeekOrigin_Start,
    SeekOrigin_End,
    SeekOrigin_Current,
    SeekOrigin_Count,
  };

  template<typename T>
  class Uninitialized {
  public:
    static inline constexpr int64_t Bytes = sizeof(T);

    T * ptr() {
      return (T *)data;
    }

    T & get() {
      return *(T *)data;
    }

    T const * ptr() const {
      return (T *)data;
    }

    T const & get() const {
      return *(T *)data;
    }

    void destruct() {
      ((T *)data)->~T();
    }

    T take() {
      T ret = std::move(get());
      destruct();
      return ret;
    }

    uint8_t data[Bytes];
  };

  template<typename T>
  int64_t read(Stream * pStream, T * pValue, int64_t count = 1);

  template<typename T>
  int64_t write(Stream * pStream, T const * pValue, int64_t count = 1);

  BFC_API int64_t read(Stream * pStream, void * pValue, int64_t size);

  BFC_API int64_t write(Stream * pStream, void const * value, int64_t size);

  class BFC_API Stream {
  public:
    virtual ~Stream() = default;

    virtual bool readable() const = 0;

    virtual bool writeable() const = 0;

    virtual bool seekable() const = 0;

    virtual bool eof() const = 0;

    virtual int64_t write(void const * data, int64_t length);

    virtual int64_t read(void * data, int64_t length);

    virtual int64_t tell() const = 0;

    virtual bool seek(int64_t pos, SeekOrigin origin = SeekOrigin_Current);

    virtual bool flush();

    template<typename T>
    bool write(T const & value) {
      return ::bfc::write(this, &value, 1) == 1;
    }

    template<typename T>
    int64_t write(T const * pValue, int64_t count) {
      return ::bfc::write(this, pValue, count);
    }

    template<typename T>
    int64_t read(T * pValue, int64_t count = 1) {
      return ::bfc::read(this, pValue, count);
    }
  };

  class BFC_API MemoryStream : public Stream {
  public:
    using Stream::write;
    using Stream::read;

    MemoryStream();
    MemoryStream(Vector<uint8_t> data, SeekOrigin origin = SeekOrigin_Start);

    virtual bool readable() const override;
    virtual bool writeable() const override;
    virtual bool seekable() const override;

    virtual bool    eof() const override;
    virtual bool    seek(int64_t pos, SeekOrigin origin) override;
    virtual int64_t tell() const override;

    virtual int64_t write(void const * data, int64_t length) override;
    virtual int64_t read(void * data, int64_t length) override;

    const Vector<uint8_t> & storage() const;

    void clear();

  private:
    Vector<uint8_t> m_data;
    int64_t         m_streamPos = 0; // // Check constructor if re-ordered...
  };

  class BFC_API MemoryReader : public Stream {
  public:
    MemoryReader(Span<uint8_t> const & data);

    virtual bool readable() const override;
    virtual bool writeable() const override;
    virtual bool seekable() const override;

    virtual bool    eof() const override;
    virtual bool    seek(int64_t pos, SeekOrigin origin) override;
    virtual int64_t tell() const override;

    virtual int64_t read(void * data, int64_t length) override;

  private:
    int64_t       m_streamPos = 0;
    Span<uint8_t> m_data;
  };

  class BFC_API TextReader {
  public:
    TextReader(Stream * pStream);

    bool       eof() const;
    StringView readLine();
    StringView read(int64_t length);
    StringView last() const;

  private:
    static constexpr int64_t ReadBufferSize = 512;

    Vector<char> m_buffer;
    char         m_readBuffer[ReadBufferSize] = {0};
    Stream *     m_pStream                    = nullptr;
  };

  class BFC_API TextWriter {
  public:
    TextWriter(Stream * pStream);

    bool writeLine(StringView const & line);
    bool write(StringView const & line);

  private:
    Stream * m_pStream = nullptr;
  };

  template<typename T>
  int64_t read(Stream * pStream, T * pValue, int64_t count) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      if constexpr (std::is_default_constructible_v<T>) {
        if constexpr (!std::is_trivially_constructible_v<T>) {
          mem::constructArray(pValue, count);
        }

        return pStream->read((void *)pValue, count * sizeof(T)) / sizeof(T);
      } else {
        static_assert(false, "T is not default constructible. read() must be implemented for T.");
      }
    } else {
      static_assert(false, "T is not trivially copyable. read() must be implemented for T.");
    }
    return 0;
  }

  template<typename T>
  int64_t write(Stream * pStream, T const * pValue, int64_t count) {
    if constexpr (std::is_trivially_copyable_v<T>) {
      return pStream->write((void const *)pValue, count * sizeof(T)) / sizeof(T);
    } else {
      static_assert(false, "T is not trivially copyable. write() must be implemented for T");
    }
    return 0;
  }

  /// Open a stream to the URI specified.
  BFC_API Ref<Stream> openURI(URI const & uri, FileMode mode);

  /// Test if a URI exists.
  BFC_API bool uriExists(URI const & uri);

  template<typename T>
  bool writeURI(URI const & uri, T const & o) {
    Ref<Stream> pStream = openURI(uri, FileMode_WriteBinary);
    if (pStream == nullptr) {
      return false;
    }
    return pStream->write(o);
  }

  template<typename T>
  std::optional<T> readURI(URI const & uri) {
    Ref<Stream> pStream = openURI(uri, FileMode_ReadBinary);
    if (pStream == nullptr) {
      return {};
    }
    Uninitialized<T> buffer;
    if (pStream->read(buffer.ptr()) != 1) {
      return {};
    }
    return std::move(buffer.get());
  }
} // namespace bfc
