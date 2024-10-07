#include "core/SerializedObject.h"
#include "core/Stream.h"

namespace bfc {
  static SerializedObject s_empty;

  SerializedObjectProxy::operator bool() const {
    return exists() && !isEmpty();
  }

  SerializedObject & SerializedObjectProxy::operator=(SerializedObject && rhs) {
    SerializedObject &        o   = resolve();
    SerializedObject::Field & fld = o.m_field;
    o.clear();
    switch (rhs.getType()) {
    case Type_Int: fld.i64 = rhs.asInt(); break;
    case Type_Float: fld.f64 = rhs.asFloat(); break;
    case Type_Text: mem::construct(&fld.text, std::move(rhs.asText())); break;
    case Type_Array: mem::construct(&fld.arr, std::move(rhs.asArray())); break;
    case Type_Map: mem::construct(&fld.map, std::move(rhs.asMap())); break;
    case Type_Empty: break;
    }
    o.m_type    = rhs.getType();
    o.m_version = rhs.m_version;
    return *this;
  }

  SerializedObject & SerializedObjectProxy::operator=(SerializedObject const & rhs) {
    SerializedObject &        o   = resolve();
    SerializedObject::Field & fld = o.m_field;
    o.clear();
    switch (rhs.getType()) {
    case Type_Int: fld.i64 = rhs.asInt(); break;
    case Type_Float: fld.f64 = rhs.asFloat(); break;
    case Type_Text: mem::construct(&fld.text, rhs.asText()); break;
    case Type_Array: mem::construct(&fld.arr, rhs.asArray()); break;
    case Type_Map: mem::construct(&fld.map, rhs.asMap()); break;
    case Type_Empty: break;
    }
    o.m_type    = rhs.getType();
    o.m_version = rhs.m_version;
    return *this;
  }

  SerializedObject & SerializedObjectProxy::resolve() {
    if (!exists()) {
      return create();
    } else {
      return get();
    }
  }

  SerializedObject const & SerializedObjectProxy::resolve() const {
    if (!exists()) {
      return s_empty;
    } else {
      return get();
    }
  }

  SerializedObjectProxy::operator SerializedObject() {
    return resolve();
  }

  SerializedObjectProxy::operator SerializedObject &() {
    return resolve();
  }

  SerializedObjectProxy::operator SerializedObject const &() const {
    return resolve();
  }

  bool SerializedObjectProxy::operator==(SerializedObject const & o) const {
    if (resolve().m_type != o.m_type) {
      return false;
    }

    switch (resolve().m_type) {
    case Type_Int: return asInt() == o.asInt();
    case Type_Float: return asFloat() == o.asFloat();
    case Type_Text: return asText() == o.asText();
    case Type_Array: return asArray() == o.asArray();
    case Type_Map: return asMap() == o.asMap();
    case Type_Empty: return true;
    }

    return false;
  }
  bool SerializedObjectProxy::operator!=(SerializedObject const & o) const {
    return !(*this == o);
  }

  SerializedObject::Type SerializedObjectProxy::getType() const {
    return resolve().m_type;
  }

  bool SerializedObjectProxy::isArray() const {
    return getType() == Type_Array;
  }

  bool SerializedObjectProxy::isMap() const {
    return getType() == Type_Map;
  }

  bool SerializedObjectProxy::isFloat() const {
    return getType() == Type_Float;
  }

  bool SerializedObjectProxy::isInt() const {
    return getType() == Type_Int;
  }

  bool SerializedObjectProxy::isText() const {
    return getType() == Type_Text;
  }

  bool SerializedObjectProxy::isEmpty() const {
    return getType() == Type_Empty;
  }

  int64_t SerializedObjectProxy::size() const {
    if (isArray()) {
      return asArray().size();
    } else if (isMap()) {
      return asMap().size();
    } else {
      return 0;
    }
  }

  Vector<SerializedObject> const & SerializedObjectProxy::asArray() const {
    BFC_ASSERT(isArray(), "Serialized data is not an Array.");
    return resolve().m_field.arr;
  }

  Map<String, SerializedObject> const & SerializedObjectProxy::asMap() const {
    BFC_ASSERT(isMap(), "Serialized data is not a Map.");
    return resolve().m_field.map;
  }

  Vector<SerializedObject> & SerializedObjectProxy::asArray() {
    BFC_ASSERT(isArray(), "Serialized data is not an Array.");
    return resolve().m_field.arr;
  }

  Map<String, SerializedObject> & SerializedObjectProxy::asMap() {
    BFC_ASSERT(isMap(), "Serialized data is not a Map.");
    return resolve().m_field.map;
  }

  int64_t SerializedObjectProxy::asInt() const {
    BFC_ASSERT(isInt(), "Serialized data is not an int.");
    return resolve().m_field.i64;
  }

  double SerializedObjectProxy::asFloat() const {
    BFC_ASSERT(isFloat(), "Serialized data is not a float.");
    return resolve().m_field.f64;
  }

  StringView SerializedObjectProxy::asText() const {
    BFC_ASSERT(isText(), "Serialized data is not text.");
    return resolve().m_field.text;
  }

  SerializedObject & SerializedObjectProxy::pushBack(SerializedObject const & o) {
    return insertAt(isArray() ? size() : 0, o);
  }

  SerializedObject & SerializedObjectProxy::pushFront(SerializedObject const & o) {
    return insertAt(0, o);
  }

  SerializedObject & SerializedObjectProxy::insertAt(int64_t const & index, SerializedObject const & o) {
    resolve().ensureType(Type_Array);
    auto & arr = asArray();
    if (index < arr.size()) {
      arr.insert(index, o);
    } else {
      arr.resize(index + 1);
      arr[index] = o;
    }
    return asArray()[index];
  }

  SerializedArrayElementProxy SerializedObjectProxy::at(int64_t const & index) {
    return SerializedArrayElementProxy(&resolve(), index);
  }

  SerializedObject const & SerializedObjectProxy::at(int64_t const & index) const {
    if (isArray() && index < asArray().size()) {
      return asArray()[index];
    }
    return s_empty;
  }

  SerializedObject & SerializedObjectProxy::add(String const & name, SerializedObject const & o) {
    resolve().ensureType(Type_Map);
    SerializedObject & entry = asMap().getOrAdd(name);
    entry                    = o;
    return entry;
  }

  SerializedMapElementProxy SerializedObjectProxy::get(String const & name) {
    return SerializedMapElementProxy(&resolve(), name);
  }

  SerializedObject const & SerializedObjectProxy::get(String const & name) const {
    if (isMap()) {
      SerializedObject const * pObject = asMap().tryGet(name);

      if (pObject != nullptr) {
        return *pObject;
      }
    }
    return s_empty;
  }

  int64_t SerializedObjectProxy::getVersion() const {
    return resolve().m_version;
  }

  void SerializedObjectProxy::setVersion(int64_t const & version) {
    resolve().m_version = version;
  }

  SerializedObject const & SerializedObjectProxy::Empty() {
    static SerializedObject empty;
    return empty;
  }

  SerializedArrayElementProxy::SerializedArrayElementProxy(SerializedObject * pParent, int64_t index)
    : m_pParent(pParent)
    , m_index(index) {}

  bool SerializedArrayElementProxy::exists() const {
    return m_pParent->isArray() && m_index >= 0 && m_index < m_pParent->size();
  }

  SerializedObject const & SerializedArrayElementProxy::get() const {
    return m_pParent->asArray()[m_index];
  }

  SerializedObject & SerializedArrayElementProxy::get() {
    return m_pParent->asArray()[m_index];
  }

  SerializedObject & SerializedArrayElementProxy::create() {
    return m_pParent->insertAt(m_index, {});
  }

  SerializedMapElementProxy::SerializedMapElementProxy(SerializedObject * pParent, String key)
    : m_pParent(pParent)
    , m_key(key) {}

  bool SerializedMapElementProxy::exists() const {
    return m_pParent->isMap() && m_pParent->asMap().contains(m_key);
  }

  SerializedObject const & SerializedMapElementProxy::get() const {
    return m_pParent->asMap()[m_key];
  }

  SerializedObject & SerializedMapElementProxy::get() {
    return m_pParent->asMap()[m_key];
  }

  SerializedObject & SerializedMapElementProxy::create() {
    return m_pParent->add(m_key, {});
  }

  SerializedObject SerializedObject::MakeArray(Vector<SerializedObject> const & arr) {
    return SerializedObject(arr);
  }

  SerializedObject SerializedObject::MakeMap(Map<String, SerializedObject> const & map) {
    return SerializedObject(map);
  }

  SerializedObject SerializedObject::MakeText(String const & value) {
    return SerializedObject(value);
  }

  SerializedObject SerializedObject::MakeInt(int64_t const & value) {
    return SerializedObject(value);
  }

  SerializedObject SerializedObject::MakeFloat(double const & value) {
    return SerializedObject(value);
  }

  SerializedObject SerializedObject::Empty() {
    return SerializedObject();
  }

  SerializedObject::SerializedObject()
    : m_field()
    , m_type(Type_Empty) {}

  SerializedObject::SerializedObject(int64_t const & value)
    : m_field(value)
    , m_type(Type_Int) {}

  SerializedObject::SerializedObject(double const & value)
    : m_field(value)
    , m_type(Type_Float) {}

  SerializedObject::SerializedObject(String const & text)
    : m_field(text)
    , m_type(Type_Text) {}

  SerializedObject::SerializedObject(Vector<SerializedObject> const & arr)
    : m_field(arr)
    , m_type(Type_Array) {}

  SerializedObject::SerializedObject(Map<String, SerializedObject> const & map)
    : m_field(map)
    , m_type(Type_Map) {}

  SerializedObject::SerializedObject(SerializedObject && rhs)
    : SerializedObject() {
    *this = std::move(rhs);
  }

  SerializedObject::SerializedObject(SerializedObject const & rhs)
    : SerializedObject() {
    *this = rhs;
  }

  SerializedObject & SerializedObject::operator=(SerializedObject const & o) {
    return SerializedObjectProxy::operator=(o);
  }

  SerializedObject & SerializedObject::operator=(SerializedObject && o) {
    return SerializedObjectProxy::operator=(std::move(o));
  }

  SerializedObject ::~SerializedObject() {
    clear();
  }

  void SerializedObject::ensureType(Type type) {
    if (getType() == type) {
      return;
    }

    switch (type) {
    case Type_Empty: break;
    case Type_Int: break;
    case Type_Float: break;
    case Type_Text: mem::construct(&m_field.text); break;
    case Type_Array: mem::construct(&m_field.arr); break;
    case Type_Map: mem::construct(&m_field.map); break;
    }

    m_type = type;
  }

  void SerializedObject::clear() {
    switch (getType()) {
    case Type_Empty: break;
    case Type_Int: break;
    case Type_Float: break;
    case Type_Text: mem::destruct(&m_field.text); break;
    case Type_Array: mem::destruct(&m_field.arr); break;
    case Type_Map: mem::destruct(&m_field.map); break;
    }

    memset(&m_field, 0, sizeof(m_field));
  }

  SerializedObject::Field::Field() {
    memset(this, 0, sizeof(*this));
  }

  SerializedObject::Field::~Field() {}

  SerializedObject::Field::Field(Map<String, SerializedObject> const & map)
    : map(map) {}

  SerializedObject::Field::Field(Vector<SerializedObject> const & vec)
    : arr(vec) {}

  SerializedObject::Field::Field(int64_t const & value)
    : i64(value) {}

  SerializedObject::Field::Field(double const & value)
    : f64(value) {}

  SerializedObject::Field::Field(String const & value)
    : text(value) {}

  int64_t write(Stream * pStream, SerializedObject const * pValues, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!pStream->write(pValues[i].m_type) || !pStream->write(pValues[i].m_version)) {
        return i;
      }

      bool success = true;
      switch (pValues[i].m_type) {
      case SerializedObjectProxy::Type_Text: success = pStream->write(pValues[i].m_field.text); break;
      case SerializedObjectProxy::Type_Int: success = pStream->write(pValues[i].m_field.i64); break;
      case SerializedObjectProxy::Type_Float: success = pStream->write(pValues[i].m_field.f64); break;
      case SerializedObjectProxy::Type_Array: success = pStream->write(pValues[i].m_field.arr); break;
      case SerializedObjectProxy::Type_Map: success = pStream->write(pValues[i].m_field.map); break;
      }
      if (!success) {
        return i;
      }
    }

    return count;
  }

  int64_t read(Stream * pStream, SerializedObject * pValues, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      mem::construct(pValues + i);

      if (!pStream->read(&pValues[i].m_type) || !pStream->read(&pValues[i].m_version)) {
        return i;
      }

      bool success = true;
      switch (pValues[i].m_type) {
      case SerializedObjectProxy::Type_Text: success = pStream->read(&pValues[i].m_field.text); break;
      case SerializedObjectProxy::Type_Int: success = pStream->read(&pValues[i].m_field.i64); break;
      case SerializedObjectProxy::Type_Float: success = pStream->read(&pValues[i].m_field.f64); break;
      case SerializedObjectProxy::Type_Array: success = pStream->read(&pValues[i].m_field.arr); break;
      case SerializedObjectProxy::Type_Map: success = pStream->read(&pValues[i].m_field.map); break;
      default:
        pValues[i].m_type = SerializedObjectProxy::Type_Empty;
        success = false; break;
      }

      if (!success) {
        mem::destruct(pValues + i);
        return i;
      }
    }

    return count;
  }
} // namespace bfc
