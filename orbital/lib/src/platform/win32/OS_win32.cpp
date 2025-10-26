#include "platform/os.h"

#ifdef BFC_WINDOWS
#include "ComInitHelper.h"

#include <Shobjidl.h>
#include <KnownFolders.h>
#include <filesystem>
#include <io.h>

namespace bfc {
  namespace os {
    static bool  getKnownID(FolderID folder, KNOWNFOLDERID * pID);
    static DWORD getNotifyFlags(FolderAttribute attr);

    static Filename getFolder(IKnownFolder * pFolder) {
      Filename     path;
      IShellItem * pShellItem = nullptr;
      HRESULT      hr         = pFolder->GetShellItem(0, IID_PPV_ARGS(&pShellItem));
      if (FAILED(hr)) {
        printf("IShellItem::GetShellItem failed\n");
        return "";
      }

      wchar_t * displayName = nullptr;
      if (FAILED(pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &displayName))) {
        printf("IShellItem::GetDisplayName failed\n");
        return "";
      }

      path = fromWide(displayName);
      CoTaskMemFree(displayName);
      return path;
    }

    Filename getSystemPath(FolderID folder) {
      win32::ComInitHelper comInit;

      if (folder == FolderID_Temp) {
        wchar_t buffer[MAX_PATH + 1];
        DWORD len = GetTempPathW(MAX_PATH + 1, buffer);

        return len == 0 ? "" : fromWide(buffer);
      }

      IKnownFolderManager * pManager = nullptr;
      HRESULT               hr       = CoCreateInstance(CLSID_KnownFolderManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pManager));
      if (FAILED(hr)) {
        printf("CoCreateInstance failed %d\n", hr);
        return "";
      }

      KNOWNFOLDERID knownFolderID;
      if (!getKnownID(folder, &knownFolderID)) {
        return "";
      }

      IKnownFolder * pKnownFolder;
      hr = pManager->GetFolder(knownFolderID, &pKnownFolder);
      if (FAILED(hr)) {
        printf("IKnownFolderManager::GetFolder failed\n");
        return "";
      }

      Filename path = getFolder(pKnownFolder);
      pKnownFolder->Release();
      return path;
    }

    Filename getExePath() {
      static Filename path = []() {
        Vector<char> buffer(MAX_PATH, '\0');
        do {
          GetModuleFileName(NULL, buffer.data(), (DWORD)buffer.size());
        } while (GetLastError() == ERROR_INSUFFICIENT_BUFFER);
        return Filename(buffer.data());
      }();

      return path;
    }

    Filename getAbsolutePath(Filename const & path) {
      wchar_t      buffer[MAX_PATH] = {0};
      std::wstring wide             = toWide(path.getView());
      DWORD        result           = GetFullPathNameW(wide.c_str(), MAX_PATH, buffer, NULL);

      if (result == 0) {
        return path;
      }

      if (result <= MAX_PATH) {
        return fromWide(buffer);
      }

      wchar_t * pBigBuffer = new wchar_t[(int64_t)result + 1];
      result               = GetFullPathNameW(wide.c_str(), result + 1, pBigBuffer, NULL);
      Filename ret         = result == 0 ? path : fromWide(pBigBuffer);
      delete[] pBigBuffer;
      return ret;
    }

    bool setEnvironment(String const & name, String const & value) {
      return SetEnvironmentVariableA(name.c_str(), value.c_str());
    }

    String getEnvironment(String const & name) {
      DWORD length = GetEnvironmentVariableA(name.c_str(), 0, 1);
      if (length == 0) {
        return "";
      }

      String variable;
      variable.resize(length - 1, ' ');
      if (GetEnvironmentVariableA(name.c_str(), variable.data(), length) == 0) {
        return "";
      }

      return variable;
    }

    bool remove(Filename const & path) {
      std::error_code err;
      return std::filesystem::remove_all(path.c_str(), err);
    }

    bool createFolders(Filename const & path) {
      std::error_code err;
      return std::filesystem::create_directories(path.c_str(), err);
    }

    bool copyFile(Filename const & src, Filename const & dst, bool overwrite) {
      std::error_code               err;
      std::filesystem::copy_options opts = std::filesystem::copy_options::none;
      if (overwrite)
        opts = std::filesystem::copy_options::overwrite_existing;
      return std::filesystem::copy_file(src.c_str(), dst.c_str(), opts, err);
    }

    String getCwd() {
      DWORD  len = GetCurrentDirectory(0, 0);
      String ret;
      ret.resize(len - 1, ' ');
      GetCurrentDirectory((DWORD)(ret.length() + 1), ret.c_str());
      return ret;
    }

    bool addPath(Filename const & path) {
      String currentPath = getEnvironment("PATH");
      if (!currentPath.endsWith(";")) // Add ';' separator if it is missing
        currentPath = currentPath + ";";
      currentPath        = currentPath + path.path().replace("/", "\\") + ";";
      return setEnvironment("PATH", currentPath);
    }

    Vector<Filename> getPath() {
      return getEnvironment("PATH").split(";", true).map([](String const & o) { return Filename(o); });
    }

    Filename searchInPath(Filename const & path, bool * pFound) {
      if (pFound != nullptr) {
        *pFound = true;
      }

      if (access(path, AccessFlag_Exists) == 0) {
        return path.absolute();
      }

      for (Filename const & dir : getPath()) {
        Filename testPath = dir / path;
        if (access(testPath, AccessFlag_Exists) == 0) {
          return testPath.absolute();
        }
      }

      if (pFound != nullptr) {
        *pFound = false;
      }
      return path;
    }

    int64_t access(Filename const & path, AccessFlag const & flags) {
      return _access(path.c_str(), (int)flags);
    }

    struct SystemEvent::Impl {
      HANDLE hEvent = 0;
    };

    SystemEvent::SystemEvent() {
      m_pData = new Impl;
    }

    SystemEvent::~SystemEvent() {
      delete m_pData;
    }

    bool SystemEvent::state() const {
      return WaitForSingleObjectEx(impl()->hEvent, 0, FALSE) == WAIT_OBJECT_0;
    }

    SystemEvent::Impl * SystemEvent::impl() const {
      return m_pData;
    }

    FolderNotification::FolderNotification(Filename const & directory, FolderAttribute flags) {
      impl()->hEvent         = 0;
      m_attributes    = flags;
      m_directory     = directory;
      reset();
    }

    FolderNotification::~FolderNotification() {
      if (impl()->hEvent != 0) {
        FindCloseChangeNotification(impl()->hEvent);
        impl()->hEvent = 0;
      }
    }

    Filename FolderNotification::path() const {
      return m_directory;
    }

    void FolderNotification::reset() const {
      if (impl()->hEvent != 0) {
        FindCloseChangeNotification(impl()->hEvent);
        impl()->hEvent = 0;
      }

      impl()->hEvent =
        FindFirstChangeNotification(m_directory.c_str(), (m_attributes & FolderAttribute_Recursive) > 0, getNotifyFlags(m_attributes));
    }

    FolderAttribute FolderNotification::attributes() const {
      return m_attributes;
    }

    Signal::Signal(bool initialState, bool manualReset) {
      impl()->hEvent = CreateEvent(NULL, manualReset, initialState, NULL);
    }

    Signal::~Signal() {
      CloseHandle(impl()->hEvent);
      impl()->hEvent = 0;
    }

    void Signal::signal() {
      SetEvent(impl()->hEvent);
    }

    void Signal::reset() const {
      ResetEvent(impl()->hEvent);
    }

    Vector<int64_t> waitFor(Vector<SystemEvent *> const & events, bool waitForAll, Timestamp timeout) {
      Vector<HANDLE>  handles  = events.map([](SystemEvent * pNotification) { return pNotification->impl()->hEvent; });
      DWORD           signaled = WaitForMultipleObjects((DWORD)handles.size(), handles.data(), FALSE, (DWORD)timeout.millis());
      Vector<int64_t> ret;
      ret.reserve(handles.size());
      for (int64_t i = signaled; i < handles.size(); ++i) {
        if (WaitForSingleObjectEx(handles[i], 0, FALSE) == WAIT_OBJECT_0) {
          events[i]->reset();
          ret.pushBack(i);
        }
      }
      return ret;
    }

    bool waitFor(SystemEvent * const & pEvent, Timestamp timeout) {
      return WaitForSingleObjectEx(pEvent->impl()->hEvent, (DWORD)timeout.millis(), FALSE) == WAIT_OBJECT_0;
    }

    bool getKnownID(FolderID folder, KNOWNFOLDERID * pID) {
      switch (folder) {
      case FolderID_AppData: *pID = FOLDERID_RoamingAppData; break;
      case FolderID_Documents: *pID = FOLDERID_Documents; break;
      default: return false;
      }
      return true;
    }

    DWORD getNotifyFlags(FolderAttribute attr) {
      DWORD flags = 0;
      if (attr & FolderAttribute_Files)
        flags |= FILE_NOTIFY_CHANGE_FILE_NAME;
      if (attr & FolderAttribute_Folders)
        flags |= FILE_NOTIFY_CHANGE_DIR_NAME;
      return flags;
    }
  } // namespace os
} // namespace bfc

#endif
