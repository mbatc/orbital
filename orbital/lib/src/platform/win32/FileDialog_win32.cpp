#include "core/Core.h"

#ifdef BFC_WINDOWS

#include "platform/FileDialog.h"
#include "ComInitHelper.h"

#include <shobjidl_core.h>
#include <string>
#include <windows.h>

namespace bfc {
  struct FileDialog::Impl {
    Filename initialFilename;
    Filename initialDirectory;

    Vector<String> extensionFilter;
    Vector<String> extensionDesc;
    Vector<Filename> selected;

    bool configureDialog(IFileDialog * pSystemDialog, int options) {
      selected.clear();

      if (pSystemDialog == nullptr)
        return false;

      FILEOPENDIALOGOPTIONS currentOptions = 0;
      pSystemDialog->GetOptions(&currentOptions);
      pSystemDialog->SetOptions(currentOptions | options);

      bool setDefaultExtension = true;
      if (extensionFilter.size() == 0) {
        extensionFilter.pushBack("*");
        extensionDesc.pushBack("All");
        setDefaultExtension = false;
      }

      size_t count = (size_t)extensionDesc.size();

      std::vector<COMDLG_FILTERSPEC> filterSpecs(count, COMDLG_FILTERSPEC{});
      std::vector<std::wstring>      names(count, L"");    // Store the strings somewhere
      std::vector<std::wstring>      patterns(count, L""); // Store the strings somewhere
      std::wstring                   defaultExtension = toWide(extensionFilter.front());

      for (int64_t i = 0; i < (int64_t)count; ++i) {
        // Store strings
        names[i]    = toWide(extensionDesc[i]);
        patterns[i] = toWide(String("*.").concat(extensionFilter[i]));

        // Pass filter to WinAPI structure
        filterSpecs[i].pszName = names[i].c_str();
        filterSpecs[i].pszSpec = patterns[i].c_str();
      }

      pSystemDialog->SetFileTypes((uint32_t)count, filterSpecs.data());
      pSystemDialog->SetFileTypeIndex(0);

      if (setDefaultExtension) {
        pSystemDialog->SetDefaultExtension(defaultExtension.c_str());
      }

      return SUCCEEDED(pSystemDialog->Show(nullptr));
    }

    bool getResults(IFileOpenDialog * pDialog) {
      selected.clear();

      IShellItemArray * pItemArray = nullptr;
      if (SUCCEEDED(pDialog->GetResults(&pItemArray))) {
        DWORD count = 0;
        if (SUCCEEDED(pItemArray->GetCount(&count))) {
          for (int64_t i = 0; i < count; ++i) {
            IShellItem * pItem = nullptr;
            if (SUCCEEDED(pItemArray->GetItemAt((DWORD)i, &pItem))) {
              LPWSTR displayName = nullptr;
              if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &displayName)))
                selected.pushBack(fromWide(displayName));
            }
          }
        }
      }

      return selected.size() > 0;
    }

    bool getResult(IFileSaveDialog * pDialog) {
      selected.clear();

      IShellItem * pItem = nullptr;
      if (SUCCEEDED(pDialog->GetResult(&pItem))) {
        LPWSTR displayName = nullptr;
        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &displayName)))
          selected.pushBack(fromWide(displayName));
      }

      return true;
    }
  };
  
  FileDialog::FileDialog() {
    m_pData = new Impl;
  }

  FileDialog::~FileDialog() {
    delete m_pData;
  }

  void FileDialog::setFile(Filename const& file) {
    m_pData->initialDirectory = file.parent();
    m_pData->initialFilename  = file.name();
  }

  void FileDialog::setDirectory(Filename const& directory) {
    m_pData->initialDirectory = directory;
  }

  void FileDialog::setFilter(Vector<String> const& extensions, Vector<String> const& descriptions) {
    m_pData->extensionDesc = descriptions;
    m_pData->extensionFilter = extensions;
  }

  bool FileDialog::save() {
    win32::ComInitHelper comInit;

    bool              success       = false;
    IFileSaveDialog * pSystemDialog = nullptr;
    HRESULT           result        = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSystemDialog));
    if (SUCCEEDED(result)) {
      if (m_pData->configureDialog(pSystemDialog, FOS_OVERWRITEPROMPT | FOS_STRICTFILETYPES)) {
        if (m_pData->getResult(pSystemDialog)) {
          success = true;
        }
      }
      pSystemDialog->Release();
    } else {
      //TODO: report error, "Failed to create Save File Dialog"
    }
    return success;
  }

  bool FileDialog::open(bool multiselect) {
    win32::ComInitHelper comInit;

    bool              success       = false;
    IFileOpenDialog * pSystemDialog = nullptr;
    HRESULT           result        = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSystemDialog));
    if (SUCCEEDED(result)) {
       if (m_pData->configureDialog(pSystemDialog, (multiselect * FOS_ALLOWMULTISELECT) | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST)) {
         if (m_pData->getResults(pSystemDialog)) {
           success = true;
        }
      }
      pSystemDialog->Release();
    } else {
      // TODO: report error, "Failed to create Open File Dialog"
    }
    return success;
  }

  bool FileDialog::openFolder(bool multiselect) {
    win32::ComInitHelper comInit;

    bool              success       = false;
    IFileOpenDialog * pSystemDialog = nullptr;
    HRESULT           result        = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pSystemDialog));
    if (SUCCEEDED(result)) {
      if (m_pData->configureDialog(pSystemDialog, (multiselect * FOS_ALLOWMULTISELECT) | FOS_PICKFOLDERS | FOS_OVERWRITEPROMPT)) {
        if (m_pData->getResults(pSystemDialog)) {
          success = true;
        }
      }
      pSystemDialog->Release();
    } else {
      // TODO: report error, "Failed to create Open File Dialog"
    }
    return success;
  }

  Span<Filename> FileDialog::getSelected() const {
    return m_pData->selected;
  }
} // namespace bfc

#endif
