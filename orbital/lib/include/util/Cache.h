#pragma once

#include "../core/Filename.h"
#include "../core/File.h"
#include "../core/Map.h"
#include "UUID.h"

#include <mutex>

namespace bfc {
  class BFC_API Cache {
  public:
    constexpr static inline int64_t Version = 1; // Version of this cache implementation

    struct Entry {
      UUID     uuid;
      int64_t  timestamp = 0;
      int64_t  version   = 0;
      File     stream;
    };

    /// Create or open a cache at `path`.
    Cache(Filename const & path);

    /// Destruct the cache.
    /// This does not remove the cache from disk.
    /// This will close all open streams and remove any non-committed entries.
    ~Cache();

    /// Check if the cache contains an entry.
    bool contains(StringView const & identifier) const;

    /// Find an entry in the cache.
    bool checkout(StringView const & identifier, Entry *pEntry) const;

    /// Create a new cache entry.
    /// This does not add it to the cache. Call commit to finalize the entry.
    Entry create() const;

    /// Submit the entry to the cache.
    void commit(StringView const & identifier, Entry *pEntry);

    /// Remove an entry from the cache.
    void remove(StringView const & identifier);

  private:
    struct OpenInfo {
      std::atomic_int64_t refs;
      Filename            path;
    };

    struct HeaderItem {
      String  id;
      UUID    uuid;
      int64_t timestamp = 0;
      int64_t version   = 0;
    };

    bool readHeaderItem(HeaderItem * pItem);
    bool writeHeaderItem(HeaderItem const &item);

    Map<String, HeaderItem> m_entries;

    File     m_header;
    Filename m_path;

    std::mutex *m_pLock = nullptr;
  };
}
