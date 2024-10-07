#include "mesh/Mesh.h"
#include "core/File.h"
#include "mesh/parsers/OBJParser.h"
#include "mesh/parsers/FBXParser.h"

namespace bfc {
  const StringView MeshData::Material::Phong::diffuse       = "baseColour";
  const StringView MeshData::Material::Phong::ambient       = "ambient";
  const StringView MeshData::Material::Phong::specular      = "specular";
  const StringView MeshData::Material::Phong::alpha         = "alpha";
  const StringView MeshData::Material::Phong::specularPower = "specularPower";
  const StringView MeshData::Material::Phong::normal        = "normal";

  const StringView MeshData::Material::PBR::albedo    = "baseColour";
  const StringView MeshData::Material::PBR::ambient   = "ambient";
  const StringView MeshData::Material::PBR::normal    = "normal";
  const StringView MeshData::Material::PBR::roughness = "roughness";
  const StringView MeshData::Material::PBR::metalness = "metalness";
  const StringView MeshData::Material::PBR::emissive = "emissive";
  const StringView MeshData::Material::PBR::alpha = "alpha";
  const StringView MeshData::Material::PBR::ao = "ao";

  void MeshData::Material::setName(StringView name) {
    m_name = name;
  }

  void MeshData::Material::setColour(StringView name, Vec4 colour, int64_t layer) {
    m_colours.addOrSet({ name, layer }, colour);
  }

  void MeshData::Material::setValue(StringView name, double value, int64_t layer) {
    m_values.addOrSet({ name, layer }, value);
  }

  void MeshData::Material::setTexture(StringView name, StringView texture, int64_t layer) {
    m_textures.addOrSet({ name, layer }, texture);
  }

  StringView MeshData::Material::getName() const {
    return m_name;
  }

  Vec4 MeshData::Material::getColour(StringView name, int64_t layer, Vec4 defaultValue) const {
    return m_colours.getOr({name, layer}, defaultValue);
  }

  double MeshData::Material::getValue(StringView name, int64_t layer, double defaultValue) const {
    return m_values.getOr({name, layer}, defaultValue);
  }

  StringView MeshData::Material::getTexture(StringView name, int64_t layer) const {
    String const* pTex = m_textures.tryGet({ name, layer });
    return pTex != nullptr ? pTex->getView() : StringView();
  }

  bool MeshData::Material::hasColour(StringView name, int64_t layer) const {
    return m_textures.contains({ name, layer });
  }

  bool MeshData::Material::hasValue(StringView name, int64_t layer) const {
    return m_values.contains({ name, layer });
  }

  bool MeshData::Material::hasTexture(StringView name, int64_t layer) const {
    return m_textures.contains({ name, layer });
  }

  Vector<MeshData::Material::PropertyID> MeshData::Material::getTextures() const {
    return m_textures.getKeys();
  }

  Vector<MeshData::Material::PropertyID> MeshData::Material::getValues() const {
    return m_values.getKeys();
  }

  Vector<MeshData::Material::PropertyID> MeshData::Material::getColours() const {
    return m_colours.getKeys();
  }
  
  bool MeshData::Material::read(URI const & uri) {
    Ref<Stream> pStream = openURI(uri, FileMode_ReadBinary);
    if (pStream == nullptr) {
      return false;
    }
    return pStream->read(this);
  }

  bool MeshData::Material::write(URI const & uri) {
    Ref<Stream> pStream = openURI(uri, FileMode_WriteBinary);
    if (pStream == nullptr) {
      return false;
    }
    return pStream->write(*this);
  }

  MeshData::MeshData(URI const & uri) {
    read(uri);
  }

  bool MeshData::canRead(StringView const & extension) {
    return extension.equals("obj", true) || extension.equals("fbx", true);
  }

  bool MeshData::read(URI const & uri) {
    Ref<Stream> pStream = openURI(uri, FileMode_ReadBinary);

    if (pStream == nullptr) {
      return false;
    }

    *this = MeshData();

    bool     success = false;
    Filename path = uri.path();
    if (path.extension().equals("obj", true)) {
      success = OBJParser::read(pStream.get(), this, path.parent());
    } else if (path.extension().equals("fbx", true)) {
      success = FBXParser::read(pStream.get(), this, path.parent());
    }

    if (success) {
      sourceFile = path;
    }

    return success;
  }

  bool MeshData::write(URI const & uri) {
    return false;
  }

  void MeshData::validate() {
    for (MeshData::Triangle& tri : triangles) {
      if (tri.material < 0 || tri.material >= materials.size()) {
        tri.material = -1;
      }
      for (int64_t& vert : tri.vertex) {
        if (vert < 0 || vert >= vertices.size()) {
          vert = -1;
        }
      }
    }

    for (MeshData::Vertex& vert : vertices) {
      if (vert.colour < 0 || vert.colour >= colours.size()) {
        vert.colour = -1;
      }

      if (vert.position < 0 || vert.position >= positions.size()) {
        vert.position = -1;
      }

      if (vert.normal < 0 || vert.normal >= normals.size()) {
        vert.normal = -1;
      }

      if (vert.uv < 0 || vert.uv >= uvs.size()) {
        vert.uv = -1;
      }
    }
  }

  void MeshData::addDefaults(bool validateFirst) {
    if (validateFirst)
      validate();

    int64_t defaultMaterial = -1;
    int64_t defaultColour = -1;
    int64_t defaultUV = -1;
    int64_t defaultPosition = -1;
    int64_t defaultNormal = -1;
    int64_t defaultVertex = -1;

    auto GetDefaultPosition = [&]() {
      if (defaultPosition == -1) {
        defaultPosition = positions.size();
        positions.pushBack(Vec3d(0));
      }
      return defaultPosition;
    };

    auto GetDefaultNormal = [&]() {
      if (defaultNormal == -1) {
        defaultNormal = normals.size();
        normals.pushBack(Vec3d(0));
      }
      return defaultNormal;
    };

    auto GetDefaultUV = [&]() {
      if (defaultUV == -1) {
        defaultUV = uvs.size();
        uvs.pushBack(Vec2d(0));
      }
      return defaultUV;
    };

    auto GetDefaultColour = [&]() {
      if (defaultColour == -1) {
        defaultColour = colours.size();
        colours.pushBack(Vec4d(1));
      }
      return defaultColour;
    };

    for (Vertex& vert : vertices) {
      if (vert.colour == -1) {
        vert.colour = GetDefaultColour();
      }

      if (vert.position == -1) {
        vert.position = GetDefaultPosition();
      }

      if (vert.normal == -1) {
        vert.normal = GetDefaultNormal();
      }

      if (vert.uv == -1) {
        vert.uv = GetDefaultUV();
      }
    }

    for (Triangle& tri : triangles) {
      if (tri.material == -1) {
        if (defaultMaterial == -1) {
          defaultMaterial = materials.size();
          materials.pushBack(Material());
        }

        tri.material = defaultMaterial;
      }

      for (int64_t& index : tri.vertex) {
        if (index < 0 || index >= vertices.size()) {
          if (index == -1) {
            index = vertices.size();
            Vertex vert;
            vert.colour = GetDefaultColour();
            vert.uv = GetDefaultUV();
            vert.normal = GetDefaultNormal();
            vert.position = GetDefaultPosition();
            vertices.pushBack(vert);
          }
        }
      }
    }

    calculateTangents();
  }

  void MeshData::findTextures() {
    mapTextures([=](Filename const & file, bfc::String const & /*name*/, int64_t /*layer*/) {
      return findFile(file, { sourceFile.parent() });
    });
  }

  void MeshData::mapTextures(std::function<Filename(Filename const & file, bfc::String const & name, int64_t layer)> const & func) {
    for (Material & mat : materials) {
      for (auto & [name, layer] : mat.getTextures()) {
        Filename found = func(mat.getTexture(name, layer), name, layer);
        mat.setTexture(name, found.path(), layer);
      }
    }
  }

  void MeshData::calculateTangents() {
    tangents.clear();
    for (Vertex& v : vertices) {
      v.tangent = -1;
    }

    for (Triangle& tri : triangles) {
      Vertex &v0 = vertices[tri.vertex[0]];
      Vertex &v1 = vertices[tri.vertex[1]];
      Vertex &v2 = vertices[tri.vertex[2]];

      Vec3d dP0 = positions[v1.position] - positions[v0.position];
      Vec3d dP1 = positions[v2.position] - positions[v0.position];

      Vec2d dUV0 = uvs[v1.uv] - uvs[v0.uv];
      Vec2d dUV1 = uvs[v2.uv] - uvs[v0.uv];

      double f = 1.0 / (dUV0.x * dUV1.y - dUV1.x * dUV0.y);
      Vec3d tangent = f * Vec3d(
          dUV1.y * dP0.x - dUV0.y * dP1.x,
          dUV1.y * dP0.y - dUV0.y * dP1.y,
          dUV1.y * dP0.z - dUV0.y * dP1.z);

      v0.tangent = v1.tangent = v2.tangent = tangents.size();

      tangents.pushBack(tangent);
    }
  }

  void MeshData::calculateFlatNormals() {
    normals.clear();
    Vector<Vertex> newVerts;
    for (Triangle & tri : triangles) {
      Vertex & v0     = vertices[tri.vertex[0]];
      Vertex & v1     = vertices[tri.vertex[1]];
      Vertex & v2     = vertices[tri.vertex[2]];

      Vec3d a = positions[v1.position] - positions[v2.position];
      Vec3d b = positions[v1.position] - positions[v0.position];

      int64_t normalIdx = normals.size();
      normals.pushBack(glm::normalize(glm::cross(a, b)));
      for (int64_t i = 0; i < 3; ++i) {
        Vertex v = vertices[tri.vertex[i]];
        v.normal = normalIdx;
        tri.vertex[i] = newVerts.size();
        newVerts.pushBack(v);
      }
    }

    std::swap(newVerts, vertices);
  }

  void MeshData::merge(MeshData const & m) {
    int64_t posOffset = positions.size();
    int64_t nrmOffset = normals.size();
    int64_t colOffset = colours.size();
    int64_t tgtOffset = tangents.size();
    int64_t texOffset = uvs.size();
    int64_t vtxOffset = vertices.size();
    int64_t triOffset = triangles.size();
    int64_t matOffset = materials.size();

    positions.pushBack(m.positions.getView());
    normals.pushBack(m.normals.getView());
    colours.pushBack(m.colours.getView());
    tangents.pushBack(m.tangents.getView());
    uvs.pushBack(m.uvs.getView());
    vertices.pushBack(m.vertices.getView());
    triangles.pushBack(m.triangles.getView());
    materials.pushBack(m.materials.getView());

    for (MeshData::Vertex & v : vertices.getView(vtxOffset)) {
      v.colour += (v.colour != -1) * colOffset;
      v.position += (v.position != -1) * posOffset;
      v.normal += (v.normal != -1) * nrmOffset;
      v.tangent += (v.normal != -1) * tgtOffset;
      v.uv += (v.uv != -1) * texOffset;
    }

    for (MeshData::Triangle & t : triangles.getView(triOffset)) {
      t.vertex[0] += vtxOffset;
      t.vertex[1] += vtxOffset;
      t.vertex[2] += vtxOffset;
      t.material += matOffset;
    }
  }

  void MeshData::applyTransform(Mat4d const & transform) {
    transformPositions(transform);
    transformNormals(transform);
  }

  void MeshData::transformPositions(Mat4d const & transform) {
    for (Vec3d & pos : positions) {
      pos = (transform * Vec4d(pos, 1));
    }
  }

  void MeshData::transformNormals(Mat4d const & transform) {
    Mat4d normalMat = glm::transpose(glm::inverse(transform));
    for (Vec3d & nrm : normals) {
      nrm = (normalMat * Vec4d(nrm, 0));
    }
  }


  BFC_API int64_t write(Stream * pStream, MeshData::Material const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].m_name) && pStream->write(pValue[i].m_colours) && pStream->write(pValue[i].m_values) &&
            pStream->write(pValue[i].m_textures)))
        return i;
    }
    return count;
  }

  BFC_API int64_t read(Stream * pStream, MeshData::Material * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].m_name) && pStream->read(&pValue[i].m_colours) && pStream->read(&pValue[i].m_values) &&
            pStream->read(&pValue[i].m_textures)))
        return i;
    }
    return count;
  }

  int64_t write(Stream * pStream, MeshData const * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->write(pValue[i].positions) && pStream->write(pValue[i].uvs) && pStream->write(pValue[i].colours) && pStream->write(pValue[i].normals) &&
            pStream->write(pValue[i].tangents) && pStream->write(pValue[i].vertices) && pStream->write(pValue[i].triangles) &&
            pStream->write(pValue[i].materials) && pStream->write(pValue[i].sourceFile)))
        return i;
    }
    return count;
  }

  int64_t read(Stream * pStream, MeshData * pValue, int64_t count) {
    for (int64_t i = 0; i < count; ++i) {
      if (!(pStream->read(&pValue[i].positions) && pStream->read(&pValue[i].uvs) && pStream->read(&pValue[i].colours) && pStream->read(&pValue[i].normals) &&
            pStream->read(&pValue[i].tangents) && pStream->read(&pValue[i].vertices) && pStream->read(&pValue[i].triangles) &&
            pStream->read(&pValue[i].materials) && pStream->read(&pValue[i].sourceFile)))
        return i;
    }
    return count;
  }
}
