#pragma once

#include "../render/GraphicsDevice.h"
#include "../render/HardwareBuffer.h"
#include "../mesh/MeshData.h"
#include "../util/Iterators.h"
#include "../geometry/Box.h"

#include "../render/RendererCommon.h"

namespace bfc {
  class BFC_API Mesh {
  public:
    using Index = uint32_t;

    struct Vertex {
      Vec3 position;
      Vec3 normal;
      Vec3 tangent;
      Colour<RGBAu8> colour;
      Vec2 uv;
    };

    struct SubMesh {
      int64_t elmOffset;
      int64_t elmCount;

      geometry::Box<float> bounds;
    };

    Mesh() = default;

    Mesh(graphics::CommandList * pCmdList, MeshData const & data);

    ~Mesh();

    bool load(graphics::CommandList * pCmdList, MeshData const & data);

    void release();
    
    SubMesh const& getSubMesh(int64_t index) const;

    Vector<SubMesh> const& getSubMeshes() const;

    int64_t getSubmeshCount() const;

    int64_t getVertexCount() const;

    int64_t getIndexCount() const;

    graphics::VertexArrayRef getVertexArray() const;

    geometry::Box<float> getBounds() const;

  private:
    // Graphics resources for this mesh
    Vector<SubMesh> m_meshes;
    geometry::Boxf  m_bounds;

    graphics::BufferRef      m_vertexBuffer;
    graphics::BufferRef      m_indexBuffer;
    graphics::VertexArrayRef m_vertexArray;

    int64_t m_vertexCount = 0;
    int64_t m_indexCount = 0;

    GraphicsDevice* m_pDevice = nullptr; // The device used to load the resources
  };

  class BFC_API Material : public graphics::StructuredBuffer<renderer::PBRMaterial> {
  public:
    using LoadTextureFunc = std::function<graphics::TextureRef(MeshData::Material, String)>;

    static constexpr int64_t TextureBindPointBase = 0;

    enum TextureSlot {
      TextureSlot_BaseColour  = 0,
      TextureSlot_Ambient,
      TextureSlot_Emissive,
      TextureSlot_Roughness,
      TextureSlot_Metalness,
      TextureSlot_AO,
      TextureSlot_Alpha,
      TextureSlot_Normal,
      TextureSlot_Count,
    };

    BlendMode blendMode = BlendMode_Opaque;

    graphics::TextureRef textures[TextureSlot_Count];

    void load(graphics::CommandList * pDevice, MeshData::Material const & def);
    void load(graphics::CommandList * pDevice, MeshData::Material const & def, LoadTextureFunc textureLoader);
    void loadValues(MeshData::Material const & def);
    void loadTextures(MeshData::Material const & def, LoadTextureFunc textureLoader);
  };

  template <>
  struct VertexTraits<Mesh::Vertex> {
    static VertexInputLayout getLayout();
  };
}
