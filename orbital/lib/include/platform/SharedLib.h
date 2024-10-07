#pragma once

#include "../core/Filename.h"

#if defined(BFC_WINDOWS)
#define BFC_SHARED_LIB_EXT "dll"
#define BFC_LIB_EXT        "lib"
#elif defined(BFC_LINUX)
#define BFC_SHARED_LIB_EXT "so"
#define BFC_LIB_EXT        "so"
#elif defined(BFC_MAC)
#define BFC_SHARED_LIB_EXT "so"
#define BFC_LIB_EXT        "so"
#endif

namespace bfc {
  typedef void (*FuncPtr_t)();

  class BFC_API SharedLib {
  public:
    struct Data; // Platform impl data

    SharedLib();
    SharedLib(Filename const& path);
    SharedLib(SharedLib &&o) = delete;
    SharedLib(SharedLib const& o) = delete;
    ~SharedLib();

    bool loaded() const;

    bool loadLib(Filename const& path);

    void freeLib();

    FuncPtr_t loadFunc(String const & function);

    static bool IsSharedLib(Filename const& path);

  private:
    Data* m_pData = nullptr;
  };
}
