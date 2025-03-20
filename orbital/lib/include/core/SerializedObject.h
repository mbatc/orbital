#pragma once

#include "Map.h"
#include "String.h"

namespace bfc {
  template<typename T>
  struct Serializer;
  struct DefaultSerializerContext {};

  class URI;
  class SerializedObject;
  class SerializedMapElementProxy;
  class SerializedArrayElementProxy;

  class BFC_API SerializedObjectProxy {
  protected:
    virtual bool exists() const = 0;

    virtual SerializedObject &       create()    = 0;
    virtual SerializedObject &       get()       = 0;
    virtual SerializedObject const & get() const = 0;

  public:
    SerializedObject &       resolve();
    SerializedObject const & resolve() const;

    virtual ~SerializedObjectProxy() = default;

    enum Type {
      Type_Empty,
      Type_Int,
      Type_Float,
      Type_Text,
      Type_Array,
      Type_Map,
      Type_Count,
    };

    template<typename T>
    SerializedObject & operator=(T const & o) {
      *this = SerializedObject(o);
      return *this;
    }

    SerializedObject & operator=(SerializedObject && rhs);
    SerializedObject & operator=(SerializedObject const & rhs);

    template<typename T, typename Context = DefaultSerializerContext>
    void write(T const & o, Context const & ctx = {}) {
      *this = Serializer<T>::write(o, ctx);
    }

    template<typename T, typename Context = DefaultSerializerContext>
    bool read(T & o, Context const & ctx = {}) const {
      return Serializer<T>::read(*this, o, ctx);
    }

    /// Read this SerializedObject as a `T`.
    /// If read fails, `o` is constructed using `constructorArgs`.
    /// @param o The uninitialized object to read or construct into.
    /// @param constructorArgs The arguments to be forwarded to T's constructor if reading the data fails.
    template<typename T, typename... Args>
    void readOrConstruct(T & o, Args&&... constructorArgs) const {
      if (!read(o)) {
        mem::construct(&o, std::forward<Args>(constructorArgs)...);
      }
    }

    operator bool() const;
    operator SerializedObject();
    operator SerializedObject &();
    operator SerializedObject const &() const;

    bool operator==(SerializedObject const & o) const;
    bool operator!=(SerializedObject const & o) const;

    Type getType() const;
    bool isArray() const;
    bool isMap() const;
    bool isFloat() const;
    bool isInt() const;
    bool isText() const;
    bool isEmpty() const;

    int64_t size() const;

    Vector<SerializedObject> const &      asArray() const;
    Map<String, SerializedObject> const & asMap() const;

    Vector<SerializedObject> &      asArray();
    Map<String, SerializedObject> & asMap();

    int64_t    asInt() const;
    double     asFloat() const;
    StringView asText() const;

    // Array interface
    SerializedObject & pushBack(SerializedObject const & o = Empty());
    SerializedObject & pushFront(SerializedObject const & o = Empty());
    SerializedObject & insertAt(int64_t const & index, SerializedObject const & o = Empty());

    SerializedArrayElementProxy at(int64_t const & index);
    SerializedObject const &    at(int64_t const & index) const;

    // Map interface
    SerializedObject & add(String const & name, SerializedObject const & o = Empty());

    SerializedMapElementProxy get(String const & name);
    SerializedObject const &  get(String const & name) const;

    /// Get the version of the serialized type.
    /// If unset, defaults to 0.
    int64_t getVersion() const;

    /// Get the version of the serialized type.
    void setVersion(int64_t const & version);

  private:
    static SerializedObject const & Empty();
  };

  class BFC_API SerializedObject : public SerializedObjectProxy {
    friend SerializedObjectProxy;

    SerializedObject(Vector<SerializedObject> const & arr);
    SerializedObject(Map<String, SerializedObject> const & map);

    virtual SerializedObject const & get() const override {
      return *this;
    }

    virtual SerializedObject & get() override {
      return *this;
    }

    virtual SerializedObject & create() override {
      return *this;
    }

    virtual bool exists() const override {
      return true;
    }

  public:
    using SerializedObjectProxy::get;

    static SerializedObject MakeArray(Vector<SerializedObject> const & arr = {});
    static SerializedObject MakeMap(Map<String, SerializedObject> const & map = {});
    static SerializedObject MakeText(String const & value);
    static SerializedObject MakeInt(int64_t const & value);
    static SerializedObject MakeFloat(double const & value);
    static SerializedObject Empty();

    SerializedObject();
    SerializedObject(int64_t const & value);
    SerializedObject(double const & value);
    SerializedObject(String const & text);

    template<typename T, typename Context = DefaultSerializerContext>
    explicit SerializedObject(T const & o, Context const & ctx = {})
      : SerializedObject(Serializer<T>::write(o, ctx)) {}

    SerializedObject(SerializedObject && rhs);
    SerializedObject(SerializedObject const & rhs);
    SerializedObject & operator=(SerializedObject const & o);
    SerializedObject & operator=(SerializedObject && rhs);

    ~SerializedObject();

    friend BFC_API int64_t write(Stream * pStream, SerializedObject const * pValues, int64_t count);
    friend BFC_API int64_t read(Stream * pStream, SerializedObject * pValues, int64_t count);

  private:
    void ensureType(Type type);
    void clear();

    Type m_type = Type_Empty;

    union Field {
      Field();
      Field(Map<String, SerializedObject> const & map);
      Field(Vector<SerializedObject> const & vec);
      Field(int64_t const & value);
      Field(double const & value);
      Field(String const & value);
      ~Field();

      Map<String, SerializedObject> map;
      Vector<SerializedObject>      arr;

      int64_t i64;
      double  f64;
      String  text;
    } m_field;

    int64_t m_version = 0;
  };

  class SerializedArrayElementProxy : public SerializedObjectProxy {
  public:
    using SerializedObjectProxy::operator=;
    using SerializedObjectProxy::operator bool;
    using SerializedObjectProxy::operator SerializedObject;
    using SerializedObjectProxy::operator SerializedObject &;
    using SerializedObjectProxy::operator SerializedObject const &;

    SerializedArrayElementProxy(SerializedObject * pParent, int64_t index);

    virtual SerializedObject const & get() const override;
    virtual SerializedObject &       get() override;
    virtual SerializedObject &       create() override;

    virtual bool exists() const override;

  private:
    int64_t            m_index   = -1;
    SerializedObject * m_pParent = nullptr;
  };

  class SerializedMapElementProxy : public SerializedObjectProxy {
  public:
    using SerializedObjectProxy::operator=;
    using SerializedObjectProxy::operator bool;
    using SerializedObjectProxy::operator SerializedObject;
    using SerializedObjectProxy::operator SerializedObject &;
    using SerializedObjectProxy::operator SerializedObject const &;

    SerializedMapElementProxy(SerializedObject * pParent, String key);

    virtual SerializedObject const & get() const override;
    virtual SerializedObject &       get() override;
    virtual SerializedObject &       create() override;
    virtual bool                     exists() const override;

  private:
    String             m_key;
    SerializedObject * m_pParent = nullptr;
  };

  BFC_API int64_t write(Stream * pStream, SerializedObject const * pValues, int64_t count);
  BFC_API int64_t read(Stream * pStream, SerializedObject * pValues, int64_t count);
} // namespace bfc
