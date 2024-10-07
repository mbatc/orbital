#include "util/Cache.h"
#include "core/File.h"

#include <filesystem>

namespace bfc {
  Cache::Cache(Filename const & path)
    : m_path(path)
    , m_pLock(new std::mutex) {
    Filename headerPath = path / "header";

    // Read the cache header
    if (m_header.open(headerPath, FileMode_ReadBinary)) {
      int64_t diskVersion = 0;
      if (m_header.read(&diskVersion)) {
        // Check if the version is correct
        if (Version == diskVersion) {
          HeaderItem item;
          while (readHeaderItem(&item)) {
            // Discard the header entry if the file doesn't exist
            if (fileExists(m_path / "data" / item.uuid.toString())) {
              m_entries.addOrSet(item.id, item);
            }
          }
        }
      }
      m_header.close();
    }

    // Write the pruned header to disk
    m_header.open(headerPath, FileMode_WriteBinary);
    m_header.write(Version);
    for (auto& [id, item] : m_entries) {
      writeHeaderItem(item);
    }
    m_header.flush();

    std::filesystem::create_directories(m_path.c_str());
    // Delete any previously staged items
    std::filesystem::remove_all((m_path / "staged").c_str());
    // Make sure the data directory exists
    std::filesystem::create_directories((m_path / "data").c_str());

    // Clean up any cached items that are not in the header
    Map<String, String> idLookup;
    for (auto & [id, item] : m_entries) {
      idLookup.add(item.uuid.toString(), id);
    }
    for (auto & item : std::filesystem::directory_iterator((m_path / "data").c_str())) {
      std::filesystem::path name = item.path().filename();
      
      String uuid = name.string().c_str();
      if (!idLookup.contains(uuid)) {
        std::filesystem::remove(item);
      }
    }

    std::filesystem::create_directories((m_path / "staged").c_str());
  }

  Cache::~Cache() {
    m_header.close();
    std::filesystem::remove_all((m_path / "staged").c_str());
    delete m_pLock;
  }

  bool Cache::contains(StringView const & identifier) const {
    std::scoped_lock lock(*m_pLock);
    return m_entries.contains(identifier);
  }

  bool Cache::checkout(StringView const & identifier, Entry * pEntry) const {
    {
      std::scoped_lock lock(*m_pLock);
      HeaderItem const * pHeader = m_entries.tryGet(identifier);

      if (pHeader == nullptr) {
        return false;
      }

      pEntry->timestamp = pHeader->timestamp;
      pEntry->uuid      = pHeader->uuid;
      pEntry->version   = pHeader->version;
    }

    return pEntry->stream.open(m_path / "data" / pEntry->uuid.toString(), FileMode_ReadBinary);
  }

  Cache::Entry Cache::create() const {
    Entry entry;
    entry.uuid       = UUID::New();
    bool opened = entry.stream.open(m_path / "staged" / entry.uuid.toString(), FileMode_WriteBinary);
    BFC_ASSERT(opened, "Failed to open stream");
    return entry;
  }

  void Cache::commit(StringView const & identifier, Entry *pEntry) {
    HeaderItem item;
    item.id        = identifier;
    item.timestamp = pEntry->timestamp;
    item.uuid      = pEntry->uuid;
    item.version   = pEntry->version;
    pEntry->stream.close();

    String uuidStr = item.uuid.toString();

    std::error_code       err;
    std::filesystem::path srcPath = (m_path / "staged" / uuidStr).c_str();
    std::filesystem::path dstPath = (m_path / "data" / uuidStr).c_str();
    std::filesystem::rename(srcPath, dstPath, err);

    std::scoped_lock lock(*m_pLock);
    if (!err) {
      {
        HeaderItem * pExisting = m_entries.tryGet(identifier);
        if (pExisting != nullptr) {
          std::filesystem::path path = (m_path / "data" / pExisting->uuid.toString()).c_str();
          std::filesystem::remove(path);
        }
      }

      m_entries.addOrSet(identifier, item);
      writeHeaderItem(item);
    }
  }

  void Cache::remove(StringView const & identifier) {
    UUID id;
    {
      std::scoped_lock   lock(*m_pLock);
      HeaderItem const *     pHeader = m_entries.tryGet(identifier);
      if (pHeader == nullptr) {
        return;
      }
      id = pHeader->uuid;
      m_entries.erase(identifier);
    }
    std::filesystem::remove((m_path / "data" / id.toString()).c_str());
  }

  bool Cache::readHeaderItem(HeaderItem *pItem) {
    return m_header.read(&pItem->id)
      && m_header.read(&pItem->timestamp)
      && m_header.read(&pItem->version)
      && m_header.read(&pItem->uuid);
  }

  bool Cache::writeHeaderItem(HeaderItem const & item) {
    int64_t start = m_header.tell();
    if (m_header.write(item.id)
      && m_header.write(item.timestamp)
      && m_header.write(item.version)
      && m_header.write(item.uuid)) {
      m_header.flush();
      return true;
    }

    m_header.seek(start, SeekOrigin_Start);
    return false;
  }
} // namespace bfc
