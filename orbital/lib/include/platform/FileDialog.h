#pragma once
#pragma once

#include "core/Filename.h"

namespace bfc {
  class BFC_API FileDialog {
    struct Impl;

  public:
    FileDialog();

    ~FileDialog();

    void setFile(Filename const & file);

    void setDirectory(Filename const & directory);

    void setFilter(Vector<String> const & extensions, Vector<String> const & descriptions);

    bool save();

    bool open(bool multiselect = false);

    bool openFolder(bool multiselect = false);

    Span<Filename> getSelected() const;

  private:
    Impl * m_pData = nullptr;
  };
}
