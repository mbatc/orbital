#include "VirtualFileSystem.h"
#include "core/File.h"
#include "util/Log.h"

using namespace bfc;

namespace engine {
  VirtualFileSystem::VirtualFileSystem(URI const & gameAssetsRoot, URI const & engineAssetsRoot)
    : Subsystem(TypeID<VirtualFileSystem>(), "VirtualFileSystem") {
    mountDrive("game", gameAssetsRoot);
    mountDrive("engine", engineAssetsRoot);
  }

  bool VirtualFileSystem::mountDrive(StringView const & name, URI const & target) {
    std::scoped_lock guard{ m_lock };
    if (!m_drives.tryAdd(name, target)) {
      return false;
    }

    BFC_LOG_INFO("VirtualFileSystem", "mounted drive (name=%s, target=%s)", name, target);
    return true;
  }

  bool VirtualFileSystem::unmountDrive(StringView const & name) {
    std::scoped_lock guard{m_lock};
    if (!m_drives.erase(name)) {
      return false;
    }

    BFC_LOG_INFO("VirtualFileSystem", "unmounted drive (name=%s)", name);
    return true;
  }

  bfc::Vector<bfc::String> VirtualFileSystem::drives() const {
    std::scoped_lock guard{m_lock};

    return m_drives.getKeys();
  }

  Ref<Stream> VirtualFileSystem::open(URI const & uri, FileMode mode) const {
    URI resolved = resolveUri(uri);

    return bfc::openURI(resolved, mode);
  }


  bool VirtualFileSystem::exists(URI const & uri) const {
    URI resolved = resolveUri(uri);

    return uriExists(resolved);
  }

  URI VirtualFileSystem::resolveUri(URI const & uri) const {
    StringView path  = uri.pathView();
    StringView drive = Filename::drive(path);

    URI virtualDriveTarget;
    if (!m_drives.tryGet(drive, &virtualDriveTarget)) {
      return uri;
    }

    URI relativeURI = uri.withPath(String("./") + path.substr(drive.length() + 1));

    return virtualDriveTarget.resolveRelativeReference(relativeURI, false);
  }

  bool VirtualFileSystem::read(URI const & resource, Vector<uint8_t> * pContent, bool binary) const {
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

  bool VirtualFileSystem::readText(URI const & resource, String * pContent) const {
    Vector<uint8_t> data;
    if (!read(resource, &data, false)) {
      return false;
    }

    if (!data.empty() && data.back() != 0)
      data.pushBack(0);

    *pContent = std::move((Vector<char>&)data);
    return true;
  }

  bool VirtualFileSystem::write(URI const & resource, Span<uint8_t> const & content) const {
    auto pStream = open(resource, FileMode_WriteBinary);
    return pStream != nullptr && pStream->write(content.data(), content.size()) == content.size();
  }

  bool VirtualFileSystem::writeText(URI const & resource, StringView const & content) const {
    auto pStream = open(resource, FileMode_Write);
    return pStream != nullptr && pStream->write(content.data(), content.size()) == content.size();
  }

  bool VirtualFileSystem::remove(URI const & resource) const {
    // return deleteUri(resolveUri(resource));
    return false;
  }

  bool VirtualFileSystem::isWritable(URI const & resource) const {
    return uriExists(resolveUri(resource));
  }

  bool VirtualFileSystem::isReadable(URI const & resource) const {
    return uriExists(resolveUri(resource));
  }

  URI VirtualFileSystem::find(URI const & resource, Vector<URI> const & basePaths, bool * pResult) const {
    for (URI const& basePath : basePaths) {
      URI test = basePath.resolveRelativeReference(resource);
      if (exists(test)) {
        return test;
      }
    }

    return resource;
  }

  bool VirtualFileSystem::isLeaf(bfc::URI const & resource) const {
    return bfc::isLeaf(resolveUri(resource));
  }

  bfc::Vector<bfc::URI> VirtualFileSystem::walk(bfc::URI const & resource, bool recursive) const {
    if (resource.empty())
      return {};

    BFC_ASSERT(recursive == false, "Recursive not fully supported. Fix resolving relative references");

    bfc::URI resolved = resolveUri(resource);

    return bfc::walk(resolved, recursive).map([=](URI const & o) {
      return resource.resolveRelativeReference(Filename::name(o.pathView()));
    });
  }

  bool VirtualFileSystem::serialize(bfc::URI const & resource, SerializedObject const & o, bfc::DataFormat format) {
    return bfc::serialize(resolveUri(resource), o, format);
  }

  std::optional<SerializedObject> VirtualFileSystem::deserialize(bfc::URI const & resource, bfc::DataFormat format) {
    return bfc::deserialize(resolveUri(resource), format);
  }
}
