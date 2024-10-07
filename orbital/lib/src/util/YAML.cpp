#include "util/YAML.h"
#include "util/Scan.h"
#include "../../../../vendor/yaml-cpp/include/yaml-cpp/eventhandler.h"
#include "../../../../vendor/yaml-cpp/include/yaml-cpp/yaml.h"

namespace YAML {
  template<>
  struct convert<bfc::StringView> {
    static YAML::Node encode(bfc::StringView const & o) {
      return YAML::Node(std::string(o.begin(), o.length()));
    }
  };

  template<>
  struct convert<bfc::String> {
    static YAML::Node encode(bfc::String const & o) {
      return YAML::Node(o.getView());
    }

    static bool decode(Node const & node, bfc::String & rhs) {
      if (!node.IsScalar())
        return false;
      rhs = node.Scalar().c_str();
      return true;
    }
  };

  template<>
  struct convert<bfc::Map<bfc::String, bfc::SerializedObject>> {
    static YAML::Node encode(bfc::Map<bfc::String, bfc::SerializedObject> const & o) {
      YAML::Node node;
      for (auto & [name, elm] : o)
        node[name] = elm;
      return node;
    }

    static bool decode(Node const & node, bfc::Map<bfc::String, bfc::SerializedObject> & rhs) {
      if (!node.IsMap())
        return false;
      for (auto it = node.begin(); it != node.end(); ++it) {
        rhs.add(it->first.as<bfc::String>(), it->second.as<bfc::SerializedObject>());
      }
      return true;
    }
  };

  template<>
  struct convert<bfc::Vector<bfc::SerializedObject>> {
    static YAML::Node encode(bfc::Vector<bfc::SerializedObject> const & o) {
      YAML::Node node;
      for (bfc::SerializedObject const & elm : o)
        node.push_back(elm);
      return node;
    }

    static bool decode(Node const & node, bfc::Vector<bfc::SerializedObject> & rhs) {
      if (!node.IsSequence())
        return false;
      for (Node const & elm : node) {
        rhs.pushBack(elm.as<bfc::SerializedObject>());
      }
      return true;
    }
  };

  template<>
  struct convert<bfc::SerializedObject> {
    static YAML::Node encode(bfc::SerializedObject const & object) {
      YAML::Node node;
      bfc::Vector<bfc::String> tags;
      if (object.getVersion() != 0) {
        tags.pushBack(bfc::String::format("ver=%lld", object.getVersion()));
      }

      switch (object.getType()) {
      case bfc::SerializedObjectProxy::Type_Int:   node = object.asInt();   break;
      case bfc::SerializedObjectProxy::Type_Float: node = object.asFloat(); break;
      case bfc::SerializedObjectProxy::Type_Text:  node = object.asText();  break;
      case bfc::SerializedObjectProxy::Type_Array: node = object.asArray(); break;
      case bfc::SerializedObjectProxy::Type_Map:   node = object.asMap();   break;
      default: node = YAML::Null; break;
      }

      return node;
    }

    static bool decode(Node const & node, bfc::SerializedObject & rhs) {
      bfc::StringView tag = (node.Tag().c_str());

      if (tag.startsWith("!"))
        tag = tag.substr(1);

      for (bfc::StringView const & entry : tag.split(",", true)) {
        bfc::Vector<bfc::StringView> kvp = entry.split("=");
        if (kvp.size() != 2)
          continue;
        bfc::StringView key   = kvp[0];
        bfc::StringView value = kvp[1];

        if (key == "ver")
          rhs.setVersion(bfc::Scan::readInt(value));
      }

      switch (node.Type()) {
      case YAML::NodeType::Map:
        rhs = bfc::SerializedObject::MakeMap(node.as<bfc::Map<bfc::String, bfc::SerializedObject>>());
        return true;
      case YAML::NodeType::Sequence:
        rhs = bfc::SerializedObject::MakeArray(node.as<bfc::Vector<bfc::SerializedObject>>());
        return true;
      case YAML::NodeType::Scalar: {
        bfc::String     scalarValue = node.Scalar().c_str();
        bfc::StringView view        = scalarValue;

        if (!scalarValue.empty()) {
          double  f   = 0;
          int64_t i   = 0;
          int64_t len = 0;

          i = bfc::Scan::readInt(view, &len);
          if (len == view.length()) {
            rhs = bfc::SerializedObject::MakeInt(i);
            return true;
          }

          f = bfc::Scan::readFloat(view, &len);
          if (len == view.length()) {
            rhs = bfc::SerializedObject::MakeFloat(f);
            return true;
          }
        }

        rhs = bfc::SerializedObject::MakeText(view);
        break;
      }
      }
      return true;
    }
  };
} // namespace YAML

namespace bfc {
  class StreamBufWrapper : public std::streambuf {
  public:
    using Element = std::streambuf::char_type;
    using Traits  = std::streambuf::traits_type;

    StreamBufWrapper(Stream * pStream)
      : m_pStream(pStream) {}

    std::streamsize xsputn(const Element * s, std::streamsize n) override {
      return (std::streamsize)m_pStream->write(s, n); // returns the number of characters successfully written.
    };

    virtual Traits::int_type overflow(int_type element) override { // put a character to stream (always fail)
      m_buf = element;
      if (m_pStream->write(m_buf) == 1) {
        return traits_type::to_int_type(m_buf);
      } else {
        return Traits::eof();
      }
    }

    virtual Traits::int_type underflow() override { // get a character from stream, but don't point past it
      return m_pCurrent == m_pEnd ? Traits::eof() : *m_pCurrent;
    }

    virtual Traits::int_type uflow() override { // get a character from stream, but don't point past it
      if (m_pCurrent >= m_pEnd) {
        int64_t bytesRead = m_pStream->read(m_inputBuffer, sizeof(m_inputBuffer));
        m_pCurrent        = m_inputBuffer;
        m_pEnd            = m_inputBuffer + bytesRead;

        if (bytesRead == 0) {
          return traits_type::eof();
        }
      }

      return traits_type::to_int_type(*(m_pCurrent++));
    }

    Stream * m_pStream = nullptr;
    Element  m_buf;

    // Read pointers
    Element * m_pEnd = nullptr;
    Element * m_pCurrent = nullptr;
    Element  m_inputBuffer[1024];
  };

  std::optional<SerializedObject> readYAML(URI const & uri) {
    return readYAML(openURI(uri, FileMode_Read).get());
  }

  bool writeYAML(URI const & uri, SerializedObject const & object) {
    return writeYAML(openURI(uri, FileMode_Write).get(), object);
  }

  std::optional<SerializedObject> readYAML(Stream * pStream) {
    if (pStream == nullptr)
      return {};
    StreamBufWrapper buf(pStream);
    std::istream     is(&buf);
    try {
      return YAML::Load(is).as<SerializedObject>();
    }
    catch (YAML::TypedBadConversion<SerializedObject>) {
      return {};
    }
  }

  bool writeYAML(Stream * pStream, SerializedObject const & object) {
    if (pStream == nullptr)
      return false;
    StreamBufWrapper buf(pStream);
    std::ostream     os(&buf);
    YAML::Emitter    yaml(os);
    yaml << YAML::Node(object);
    return yaml.good();
  }
} // namespace bfc
