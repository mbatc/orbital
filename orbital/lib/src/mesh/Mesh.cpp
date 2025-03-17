#include "mesh/Mesh.h"
#include "media/Image.h"

namespace bfc {
  Mesh::Mesh(graphics::CommandList * pCmdList, MeshData const & data) {
    load(pCmdList, data);
  }

  Mesh::~Mesh() {
    release();
  }

  bool Mesh::load(graphics::CommandList * pCmdList, MeshData const & data) {
    release();

    m_pDevice                         = pCmdList->getDevice();

    m_vertexBuffer = pCmdList->createBuffer(BufferUsageHint_Vertices);
    m_indexBuffer  = pCmdList->createBuffer(BufferUsageHint_Indices);
    m_vertexArray  = pCmdList->createVertexArray();

    m_vertexCount = data.vertices.size();
    m_indexCount  = data.triangles.size() * 3;

    // Create vertex buffer
    Vector<Vertex> vertexData = data.vertices.map([&](MeshData::Vertex const & v) {
      Vertex ret;
      ret.position = data.positions[v.position];
      ret.uv       = data.uvs[v.uv];
      ret.colour   = Colour<RGBAf32>(data.colours[v.colour]);
      ret.normal   = data.normals[v.normal];
      ret.tangent  = data.tangents[v.tangent];
      return ret;
    });

    // Allocate memory for indices
    Vector<Vector<Index>> subMeshIndexData;
    subMeshIndexData.resize(data.materials.size());
    for (Vector<Index> & indices : subMeshIndexData) {
      indices.reserve(data.vertices.size() / subMeshIndexData.size());
    }

    // Group indices by material
    for (MeshData::Triangle const & tri : data.triangles) {
      Vector<Index> & indices = subMeshIndexData[tri.material];
      for (int64_t v : tri.vertex) {
        indices.pushBack((uint32_t)v);
      }
    }

    // Concatenate into a single buffer
    Vector<Index> indexData;
    indexData.resize(m_indexCount);

    Index * pBegin = indexData.begin();
    Index * pNext  = indexData.begin();
    for (Vector<Index> & indices : subMeshIndexData) {
      SubMesh sm;
      sm.elmOffset = pNext - pBegin;
      sm.elmCount  = indices.size();
      m_meshes.pushBack(sm);
      memcpy(pNext, indices.begin(), sizeof(Index) * indices.size());
      pNext += indices.size();
    }
    
    m_vertexArray->setLayout(VertexInputLayout::Create<Vertex>());
    m_vertexArray->setVertexBuffer(0, m_vertexBuffer);
    m_vertexArray->setIndexBuffer(m_indexBuffer, DataType_UInt32);

    // Calculate extents of each sub-mesh
    for (auto & [i, indices] : enumerate(subMeshIndexData)) {
      geometry::Boxf & bounds = m_meshes[i].bounds;
      bounds = geometry::Boxf();
      for (Index const & vertIndex : indices) {
        bounds.growToContain(vertexData[vertIndex].position);
      }

      m_bounds.growToContain(bounds);
    }

    pCmdList->upload(m_vertexBuffer, std::move(vertexData));
    pCmdList->upload(m_indexBuffer, std::move(indexData));

    return true;
  }

  void Mesh::release() {
    m_vertexArray  = {};
    m_vertexBuffer = {};
    m_indexBuffer  = {};
    m_vertexCount  = 0;
    m_indexCount   = 0;
    m_pDevice      = nullptr;
  }

  Mesh::SubMesh const & Mesh::getSubMesh(int64_t index) const {
    return m_meshes[index];
  }

  Vector<Mesh::SubMesh> const & Mesh::getSubMeshes() const {
    return m_meshes;
  }

  int64_t Mesh::getSubmeshCount() const {
    return m_meshes.size();
  }

  int64_t Mesh::getVertexCount() const {
    return m_vertexCount;
  }

  int64_t Mesh::getIndexCount() const {
    return m_indexCount;
  }

  graphics::VertexArrayRef Mesh::getVertexArray() const {
    return m_vertexArray;
  }

  geometry::Box<float> Mesh::getBounds() const {
    return m_bounds;
  }

  VertexInputLayout VertexTraits<Mesh::Vertex>::getLayout() {
    constexpr int64_t stride = sizeof(Mesh::Vertex);
    VertexInputLayout layout;
    layout.setAttribute("POSITION0", {0, DataType_Float32, DataClass_Vector, 3, 1, BFC_OFFSET_OF(Mesh::Vertex, position), stride, LayoutFlag_None});
    layout.setAttribute("TEXCOORD0", {0, DataType_Float32, DataClass_Vector, 2, 1, BFC_OFFSET_OF(Mesh::Vertex, uv), stride, LayoutFlag_None});
    layout.setAttribute("COLOUR0", {0, DataType_UInt8, DataClass_Vector, 4, 1, BFC_OFFSET_OF(Mesh::Vertex, colour), stride, LayoutFlag_Normalize});
    layout.setAttribute("NORMAL0", {0, DataType_Float32, DataClass_Vector, 3, 1, BFC_OFFSET_OF(Mesh::Vertex, normal), stride, LayoutFlag_None});
    layout.setAttribute("TANGENT0", {0, DataType_Float32, DataClass_Vector, 3, 1, BFC_OFFSET_OF(Mesh::Vertex, tangent), stride, LayoutFlag_None});
    return layout;
  }

  void Material::load(graphics::CommandList * pCmdList, MeshData::Material const & def, LoadTextureFunc textureLoader) {
    loadValues(def);
    loadTextures(def, textureLoader);
    upload(pCmdList);
  }

  void Material::load(graphics::CommandList * pCmdList, MeshData::Material const & def) {
    // Default texture loading function
    auto textureLoader = [pCmdList](const MeshData::Material & def, const String & textureName) {
      graphics::TextureRef pTexture;
      StringView   texPath  = def.getTexture(textureName);
      if (!graphics::loadTexture2D(pCmdList, &pTexture, texPath)) {
        return graphics::TextureRef{};
      }
      return pTexture;
    };

    return load(pCmdList, def, textureLoader);
  }

  void Material::loadValues(MeshData::Material const & def) {
    data.albedo    = def.getColour(MeshData::Material::PBR::albedo, 0, data.albedo);
    data.ambient   = def.getColour(MeshData::Material::PBR::ambient, 0, data.ambient);
    data.emissive  = def.getColour(MeshData::Material::PBR::emissive, 0, data.emissive);
    data.roughness = (float)def.getValue(MeshData::Material::PBR::roughness, 0, data.roughness);
    data.metalness = (float)def.getValue(MeshData::Material::PBR::metalness, 0, data.metalness);
    data.alpha     = (float)def.getValue(MeshData::Material::PBR::alpha, 0, data.alpha);
  }

  void Material::loadTextures(MeshData::Material const & def, LoadTextureFunc textureLoader) {
    textures[TextureSlot_BaseColour] = textureLoader(def, MeshData::Material::PBR::albedo);
    textures[TextureSlot_Ambient]    = textureLoader(def, MeshData::Material::PBR::ambient);
    textures[TextureSlot_Emissive]   = textureLoader(def, MeshData::Material::PBR::emissive);
    textures[TextureSlot_Roughness]  = textureLoader(def, MeshData::Material::PBR::roughness);
    textures[TextureSlot_Metalness]  = textureLoader(def, MeshData::Material::PBR::metalness);
    textures[TextureSlot_AO]         = textureLoader(def, MeshData::Material::PBR::ao);
    textures[TextureSlot_Alpha]      = textureLoader(def, MeshData::Material::PBR::alpha);
    textures[TextureSlot_Normal]     = textureLoader(def, MeshData::Material::PBR::normal);
  }
} // namespace bfc
