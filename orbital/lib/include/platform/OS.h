#pragma once

#include "../core/Filename.h"
#include "../core/Timestamp.h"

namespace bfc {
  namespace os {
    enum FolderID {
      FolderID_AppData,
      FolderID_Documents,
    };

    enum AccessFlag {
      AccessFlag_Exists    = 0,
      AccessFlag_Read      = 2,
      AccessFlag_Write     = 4,
      AccessFlag_ReadWrite = 6,
    };

    BFC_API Filename getSystemPath(FolderID folder);
    BFC_API Filename getExePath();
    BFC_API Filename getAbsolutePath(Filename const & path);

    BFC_API bool   remove(Filename const & path);
    BFC_API bool   createFolders(Filename const & path);
    BFC_API bool   copyFile(Filename const & src, Filename const & dst, bool overwrite = false);
    BFC_API bool   setEnvironment(String const & name, String const & value);
    BFC_API String getEnvironment(String const & name);
    BFC_API String getCwd();
    BFC_API bool   addPath(Filename const & name);
    BFC_API Vector<Filename> getPath();
    /// Search for a file relative to PATH
    BFC_API Filename searchInPath(Filename const & path, bool *pFound = nullptr);
    BFC_API int64_t access(Filename const & path, AccessFlag const & flags);

    enum FolderAttribute {
      FolderAttribute_None      = 0,
      FolderAttribute_Files     = 1 << 0,
      FolderAttribute_Folders   = 1 << 1,
      FolderAttribute_Recursive = 1 << 2,
    };

    class BFC_API SystemEvent {
    public:
      /// Os specific implementation details
      struct Impl;

      virtual ~SystemEvent();

      /// Manually reset the event.
      virtual void reset() const = 0;
      /// Get the state of the event.
      virtual bool state() const;

      /// Get the implementation.
      Impl * impl() const;

    protected:
      /// Only derived class can construct a SystemEvent
      SystemEvent();


    private:
      Impl *m_pData = nullptr;
    };

    class BFC_API FolderNotification : public SystemEvent {
    public:
      FolderNotification(Filename const & directory, FolderAttribute flags);

      ~FolderNotification();

      /// Get the path being watched.
      Filename path() const;
      /// Manually reset the event.
      virtual void reset() const override;
      /// Alias for `state` with a more intuitive name for a Folder Notification.
      inline bool changed() const {
        return state();
      }

      FolderAttribute attributes() const;

    private:
      Filename        m_directory;
      FolderAttribute m_attributes = FolderAttribute_None;
    };

    class BFC_API Signal : public SystemEvent {
    public:
      Signal(bool initialState = false, bool manualReset = false);
      ~Signal();

      /// Set the signal
      void signal();
      /// Manually reset the event.
      virtual void reset() const override;
    };

    /// Wait for multiple events.
    /// If `waitForAll` is false the function returns once one or more of the events specified have been set.
    /// If `waitForAll` is true the function returns once all the events specified have been set.
    /// @param events     The events to wait on.
    /// @param waitForAll If the function should wait until all events are signalled.
    /// @param timeout    How long to wait until timing out.
    /// @return A vector containing the indices of the signalled events.
    BFC_API Vector<int64_t> waitFor(Vector<SystemEvent *> const & events, bool waitForAll = false, Timestamp timeout = std::numeric_limits<int64_t>::max());
    /// Wait for a single event.
    /// @retval true  The event was signaled.
    /// @retval false The timeout was reached or an error occured.
    BFC_API bool waitFor(SystemEvent * const & pEvent, Timestamp timeout = std::numeric_limits<int64_t>::max());
  } // namespace os

  template<>
  struct enable_bitwise_operators<os::FolderAttribute> : std::true_type {};
} // namespace bfc
