#include "VirtualFileSystem.h"
#include "core/File.h"
#include "util/Log.h"

using namespace bfc;

namespace {
  std::mutex                      g_lock;
  bfc::Map<bfc::String, bfc::URI> g_drives;
  bfc::Map<bfc::URI, bfc::UUID>   g_virtualFiles;
}

namespace engine {
  namespace fs {
    void init(bfc::URI const & gameAssetsRoot, bfc::URI const & engineAssetsRoot) {
      mountDrive("game", gameAssetsRoot);
      mountDrive("engine", engineAssetsRoot);
    }

    bool mountDrive(StringView const & name, URI const & target) {
      std::scoped_lock guard{g_lock};
      if (!g_drives.tryAdd(name, target)) {
        return false;
      }

      BFC_LOG_INFO("VirtualFileSystem", "mounted drive (name=%s, target=%s)", name, target);
      return true;
    }

    bool unmountDrive(StringView const & name) {
      std::scoped_lock guard{g_lock};
      if (!g_drives.erase(name)) {
        return false;
      }

      BFC_LOG_INFO("VirtualFileSystem", "unmounted drive (name=%s)", name);
      return true;
    }

    bfc::Vector<bfc::String> drives() {
      std::scoped_lock guard{g_lock};

      return g_drives.getKeys();
    }

    Ref<Stream> open(URI const & uri, FileMode mode) {
      URI resolved = resolveUri(uri);

      return bfc::openURI(resolved, mode);
    }

    bool exists(URI const & uri) {
      URI resolved = resolveUri(uri);

      return uriExists(resolved);
    }

    URI resolveUri(URI const & uri) {
      StringView path  = uri.pathView();
      StringView drive = Filename::drive(path);

      URI virtualDriveTarget;
      if (!g_drives.tryGet(drive, &virtualDriveTarget)) {
        return uri;
      }

      URI relativeURI = uri.withPath(String("./") + path.substr(drive.length() + 1));

      return virtualDriveTarget.resolveRelativeReference(relativeURI, false);
    }

    bfc::URI toVirtualUri(bfc::URI const & uri) {
      for (auto & [drive, root] : g_drives) {
        bfc::URI      relativeUri  = uri.relativeTo(root);
        bfc::Filename relativePath = relativeUri.path();
        if (relativePath.isAbsolutePath() || relativePath.path().startsWith("../"))
          continue;
        return relativeUri.withPath(String::format("%s:%s", drive, relativePath));
      }
      return uri;
    }

    bool read(URI const & resource, Vector<uint8_t> * pContent, bool binary) {
      const int64_t readSize = 32 * 1024 * 1024ll;
      auto          pStream  = open(resource, binary ? FileMode_ReadBinary : FileMode_Read);
      if (pStream == nullptr) {
        return false;
      }

      int64_t   length   = 0;
      int64_t   capacity = 0;
      uint8_t * pData    = nullptr;
      while (!pStream->eof()) {
        capacity = length + readSize;
        pData    = (uint8_t *)mem::realloc(pData, capacity);
        length += pStream->read(pData, readSize);
      }

      pContent->setData(pData, length, capacity);
      return true;
    }

    bool readText(URI const & resource, String * pContent) {
      Vector<uint8_t> data;
      if (!read(resource, &data, false)) {
        return false;
      }

      if (!data.empty() && data.back() != 0)
        data.pushBack(0);

      *pContent = std::move((Vector<char> &)data);
      return true;
    }

    bool write(URI const & resource, Span<uint8_t> const & content) {
      auto pStream = open(resource, FileMode_WriteBinary);
      return pStream != nullptr && pStream->write(content.data(), content.size()) == content.size();
    }

    bool writeText(URI const & resource, StringView const & content) {
      auto pStream = open(resource, FileMode_Write);
      return pStream != nullptr && pStream->write(content.data(), content.size()) == content.size();
    }

    bool remove(URI const & resource) {
      // return deleteUri(resolveUri(resource));
      return false;
    }

    bool isWritable(URI const & resource) {
      return uriExists(resolveUri(resource));
    }

    bool isReadable(URI const & resource) {
      return uriExists(resolveUri(resource));
    }

    URI find(URI const & resource, Vector<URI> const & basePaths, bool * pResult) {
      for (URI const & basePath : basePaths) {
        URI test = basePath.resolveRelativeReference(resource);
        if (exists(test)) {
          return test;
        }
      }

      return resource;
    }

    bool isLeaf(bfc::URI const & resource) {
      return bfc::isLeaf(resolveUri(resource));
    }

    bfc::Vector<bfc::URI> walk(bfc::URI const & resource, bool recursive) {
      if (resource.empty())
        return {};

      BFC_ASSERT(recursive == false, "Recursive not fully supported. Fix resolving relative references");

      bfc::URI resolved = resolveUri(resource);

      return bfc::walk(resolved, recursive).map([=](URI const & o) {
        return resource.resolveRelativeReference(Filename::name(o.pathView()));
      });
    }

    std::optional<Timestamp> lastModified(bfc::URI const & resource) {
      return bfc::lastModified(resolveUri(resource));
    }

    bool serialize(bfc::URI const & resource, SerializedObject const & o, bfc::DataFormat format) {
      return bfc::serialize(resolveUri(resource), o, format);
    }

    std::optional<SerializedObject> deserialize(bfc::URI const & resource, bfc::DataFormat format) {
      return bfc::deserialize(resolveUri(resource), format);
    }
  } // namespace fs
}
