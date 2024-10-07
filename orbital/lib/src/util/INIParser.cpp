#include "util/INIParser.h"
#include "core/Stream.h"
#include "core/StringSplitter.h"

namespace bfc {
  BFC_API bool readINI(URI const& uri, Map<String, String>* pContent) {
    Ref<Stream> pStream = openURI(uri, FileMode_Read);
    if (pStream == nullptr) {
      return false;
    }

    TextReader     reader(pStream.get());
    StringSplitter splitter;

    String group;
    while (!reader.eof()) {
      StringView line = reader.readLine().trimStart();
      if (line.startsWith(";")) {
        continue;
      }

      if (line.startsWith("[") && line.endsWith("]")) {
        group = line.trimStart("[").trimEnd("]").trim();
        group = group + "/";
      } else {
        int64_t equal = line.find("=");
        if (equal == npos) {
          continue;
        }

        StringView key   = line.substr(0, equal);
        StringView value = line.substr(equal + 1);
        pContent->add(group + key, value);
      }
    }

    return true;
  }

  BFC_API bool writeINI(URI const & uri, const Map<String, String> & content) {
    Ref<Stream> pStream = openURI(uri, FileMode_Write);
    if (pStream == nullptr) {
      return false;
    }

    TextWriter  writer(pStream.get());
    StringSplitter splitter;
    Vector<Pair<String, String>> items = content.getItems();

    // Sort by keys
    std::sort(items.begin(), items.end(), [](Pair<String, String> const & a, Pair<String, String> const & b) { return a.first < b.first; });
    String lastGroup;
    // Write key/values to ini file
    for (auto& [key, value] : items) {
      int64_t    slash = key.findLast("/");
      StringView group = key.substr(0, slash == npos ? 0 : slash);
      StringView name  = key.substr(group.length() != 0 ? group.length() + 1 : 0);
      if (lastGroup != group) {
        if (!(writer.write("[") && writer.write(group) && writer.write("]\n")))
          return false;
        lastGroup = group;
      }

      if (!(writer.write(name) && writer.write("=") && writer.writeLine(value)))
        return false;
    }

    return true;
  }
} // namespace bfc
