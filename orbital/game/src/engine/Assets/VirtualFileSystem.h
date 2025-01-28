#pragma once

#include "../Subsystem.h"
#include "core/Memory.h"
#include "core/URI.h"
#include "core/Stream.h"
#include "core/Map.h"
#include "core/Serialize.h"
#include "util/UUID.h"

#include <optional>
#include <mutex>

namespace engine {
  class VirtualFileSystem : public Subsystem {
  public:
    VirtualFileSystem(bfc::URI const & gameAssetsRoot, bfc::URI const & engineAssetsRoot);

    /// Mount a virtual drive name.
    /// @param name The name of the drive.
    /// @param target The target URI that the name maps to.
    /// @retval true The drive name was mounted.
    /// @retval false The drive name is already mounted.
    bool mountDrive(bfc::StringView const & name, bfc::URI const & target);

    /// Unmount a virtual drive name.
    /// @param name The name of the drive.
    /// @retval true  If the drive name existed and was unmounted.
    /// @retval false If the drive name was not found.
    bool unmountDrive(bfc::StringView const & name);

    /// Get a list of drives mounted in the virtual file system.
    bfc::Vector<bfc::String> drives() const;

    /// Open a stream to the URI specified.
    bfc::Ref<bfc::Stream> open(bfc::URI const & uri, bfc::FileMode mode) const;

    /// Test if a URI exists.
    bool exists(bfc::URI const & uri) const;

    /// Resolve any virtual drives in a URI to the full target path.
    bfc::URI resolveUri(bfc::URI const & uri) const;

    /// Read a binary file.
    bool read(bfc::URI const & resource, bfc::Vector<uint8_t> * pContent, bool binary = true);

    /// Read a text file.
    bool readText(bfc::URI const & resource, bfc::String * pContent);

    /// Write a binary file.
    bool write(bfc::URI const & resource, bfc::Span<uint8_t> const & content);

    /// Write a text file.
    bool writeText(bfc::URI const & resource, bfc::StringView const & content);

    /// Remove a file.
    /// @retval true  The resource was deleted.
    /// @retval false The resource could not be deleted.
    bool remove(bfc::URI const & resource);

    /// Test if a path is writable.
    /// @retval true  The resource is writable.
    /// @retval false The resource is not writable.
    bool isWritable(bfc::URI const & resource);

    /// Test if a path is readble.
    /// @retval true  The resource is readable.
    /// @retval false The resource is not readable.
    bool isReadable(bfc::URI const & resource);

    /// Find a resource in the searchPaths provided.
    /// @retval true  The resource is readable.
    /// @retval false The resource is not readable.
    bfc::URI find(bfc::URI const & resource, bfc::Vector<bfc::URI> const & basePaths, bool * pResult = nullptr);

    template<typename T>
    bool write(bfc::URI const & resource, T const & o) {
      bfc::Ref<bfc::Stream> pStream = open(resource, bfc::FileMode_WriteBinary);

      if (pStream == nullptr) {
        return false;
      }

      return pStream->writeable() && pStream->write(o);
    }

    template<typename T>
    std::optional<T> read(bfc::URI const & resource) {
      bfc::Ref<bfc::Stream> pStream = open(resource, bfc::FileMode_ReadBinary);
      if (pStream == nullptr) {
        return {};
      }

      if (!pStream->readable()) {
        return {};
      }

      bfc::Uninitialized<T> buffer;
      if (pStream->read(buffer.ptr()) != 1) {
        return {};
      }
      return std::move(buffer.get());
    }

    template<typename T>
    bool serialize(bfc::URI const & resource, T const & o, bfc::DataFormat format = bfc::DataFormat_YAML) {
      return bfc::serialize(resolveUri(resource), o, format);
    }

    template<typename T>
    std::optional<T> deserialize(bfc::URI const & resource, bfc::DataFormat format = bfc::DataFormat_YAML) {
      return bfc::deserialize<T>(resolveUri(resource), format);
    }

    bool serialize(bfc::URI const & resource, bfc::SerializedObject const & o, bfc::DataFormat format = bfc::DataFormat_YAML);

    std::optional<bfc::SerializedObject> deserialize(bfc::URI const & resource, bfc::DataFormat format = bfc::DataFormat_YAML);

  private:
    std::mutex                      m_lock;
    bfc::Map<bfc::String, bfc::URI> m_drives;
    bfc::Map<bfc::URI, bfc::UUID>   m_virtualFiles;
  };
}
