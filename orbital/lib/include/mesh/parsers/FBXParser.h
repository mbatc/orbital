#pragma once

#include "../../core/Core.h"
#include "../../core/StringView.h"
#include "../MeshData.h"
#include "../MeshSkeleton.h"

namespace bfc {
  class Stream;
  class MeshData;

  class BFC_API FBXParser {
  public:
    struct Node {
      /// Index into to the scene material list.
      /// If a mesh triangle references material 1. The actual material is located at
      /// scene.materials[node.materials[1]]
      Vector<int64_t> materials;

      int64_t parentNode = -1;
      int64_t mesh = -1;

      Mat4d   transform;
      Mat4d   globalTransform;
    };

    struct Scene {
      Vector<Node>               nodes;
      Vector<MeshData>           meshes;
      Vector<MeshSkeleton>       skeletons;
      Vector<MeshData::Material> materials;
    };

    /// Read an FBX into a `Scene` struct.
    /// This is a compact representation of supported elements of an FBX scene.
    static bool read(Stream * pStream, Scene * pScene);

    /// Read an FBX file into a `MeshData` instance. Only scene elements that can be represented in a single mesh are returned.
    /// All instanced meshes are resolved so even a small model has the potential to use a lot of memory.
    static bool read(Stream * pStream, MeshData * pMesh, StringView const & resourceDir = "");

    static bool write(Stream * pStream, MeshData const * pMesh);
  };
} // namespace bfc
