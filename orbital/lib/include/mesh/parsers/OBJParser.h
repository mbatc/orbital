#pragma once

#include "../../core/Core.h"
#include "../../core/StringView.h"
#include "../MeshData.h"

namespace bfc {
  class Stream;
  class MeshData;

  class BFC_API OBJParser {
  public:
    static bool read(Stream* pStream, MeshData* pMesh, StringView const& resourceDir = "");
    static bool write(Stream* pStream, MeshData const* pMesh);
  };

  class BFC_API MTLParser {
  public:
    static Vector<MeshData::Material> read(Stream * pStream, Span<String> const & names = {});
    static bool                       write(Stream * pStream, Vector<MeshData::Material> const & materials);
  };
}
