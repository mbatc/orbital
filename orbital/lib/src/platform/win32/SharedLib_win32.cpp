#include "platform/SharedLib.h"

#ifdef BFC_WINDOWS

#include <Windows.h>

namespace bfc {
  struct SharedLib::Data {
    HMODULE hModule = 0;
  };

  SharedLib::SharedLib()
    : m_pData(new SharedLib::Data) {
  }

  SharedLib::SharedLib(Filename const& path)
      : SharedLib()
  {
    loadLib(path);
  }

  SharedLib::~SharedLib() {
    freeLib();
    delete m_pData->hModule;
  }

  bool SharedLib::loaded() const {
    return m_pData->hModule != 0;
  }

  bool SharedLib::loadLib(Filename const& path) {
    freeLib();
    m_pData->hModule = LoadLibraryA(path.c_str());
    return loaded();
  }

  void SharedLib::freeLib() {
    if (m_pData->hModule != 0)
      FreeLibrary(m_pData->hModule);
    m_pData->hModule = 0;
  }

  FuncPtr_t SharedLib::loadFunc(String const& function) {
    return loaded() ? (FuncPtr_t)GetProcAddress(m_pData->hModule, function.c_str()) : nullptr;
  }

  bool SharedLib::IsSharedLib(Filename const& path) {
    return path.extension() == "dll";
  }
}

#endif
