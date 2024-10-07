#include "mesh/Mesh.h"
#include "media/Image.h"

namespace bfc {
  Mesh::Mesh(GraphicsDevice * pDevice, MeshData const & data) {
    load(pDevice, data);
  }

  Mesh::~Mesh() {
    release();
  }

  bool Mesh::load(GraphicsDevice * pDevice, MeshData const & data) {
    release();

    graphics::BufferManager * pBuffer = pDevice->getBufferManager();
    m_pDevice                         = pDevice;

    m_vertexBuffer = {m_pDevice, pBuffer->createBuffer(BufferUsageHint_Vertices)};
    m_indexBuffer  = {m_pDevice, pBuffer->createBuffer(BufferUsageHint_Indices)};
    m_vertexArray  = {m_pDevice, pBuffer->createVertexArray()};

    m_vertexCount = data.vertices.size();
    m_indexCount  = data.triangles.size() * 3;

    pBuffer->upload(m_vertexBuffer, sizeof(Vertex) * data.vertices.size());
    pBuffer->upload(m_indexBuffer, sizeof(Index) * m_indexCount);

    // Create vertex buffer
    Span<Vertex> vertexData = {(Vertex *)pBuffer->map(m_vertexBuffer, MapAccess_Write), data.vertices.size()};
    for (auto & [i, vert] : enumerate(data.vertices)) {
      Vertex & v = vertexData[i];

      v.position = data.positions[vert.position];
      v.uv       = data.uvs[vert.uv];
      v.colour   = Colour<RGBAf32>(data.colours[vert.colour]);
      v.normal   = data.normals[vert.normal];
      v.tangent  = data.tangents[vert.tangent];
    }

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
    Span<Index> indexData = {(Index *)pBuffer->map(m_indexBuffer, MapAccess_Write), m_indexCount};
    Index *     pBegin    = indexData.begin();
    for (Vector<Index> & indices : subMeshIndexData) {
      SubMesh sm;
      sm.elmOffset = indexData.begin() - pBegin;
      sm.elmCount  = indices.size();
      m_meshes.pushBack(sm);
      memcpy(indexData.begin(), indices.begin(), sizeof(Index) * indices.size());
      indexData = indexData.getElements(indices.size());
    }

    pBuffer->setLayout(m_vertexArray, VertexInputLayout::Create<Vertex>());
    pBuffer->setVertexBuffer(m_vertexArray, 0, m_vertexBuffer);
    pBuffer->setIndexBuffer(m_vertexArray, m_indexBuffer, DataType_UInt32);

    // Calculate extents of each sub-mesh
    for (auto & [i, indices] : enumerate(subMeshIndexData)) {
      geometry::Boxf & bounds = m_meshes[i].bounds;
      bounds = geometry::Boxf();
      for (Index const & vertIndex : indices) {
        bounds.growToContain(vertexData[vertIndex].position);
      }

      m_bounds.growToContain(bounds);
    }

    // Unmap GPU buffers
    pBuffer->unmap(m_indexBuffer);
    pBuffer->unmap(m_vertexBuffer);
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

  GraphicsResource Mesh::getVertexArray() const {
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

  void Material::load(GraphicsDevice * pDevice, MeshData::Material const & def, LoadTextureFunc textureLoader) {
    loadValues(def);
    loadTextures(def, textureLoader);
    upload(pDevice);
  }

  void Material::load(GraphicsDevice * pDevice, MeshData::Material const & def) {
    // Default texture loading function
    auto textureLoader = [pDevice](const MeshData::Material & def, const String & textureName) {
      Ref<Texture> pTexture = NewRef<Texture>();
      StringView   texPath  = def.getTexture(textureName);
      if (!pTexture->load2D(pDevice, texPath)) {
        return Ref<Texture>();
      }
      return pTexture;
    };

    return load(pDevice, def, textureLoader);
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
