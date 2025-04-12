#include "mesh/parsers/OBJParser.h"
#include "core/File.h"
#include "core/Stream.h"
#include "core/StringSplitter.h"
#include "mesh/MeshData.h"
#include "util/Iterators.h"
#include "util/Scan.h"

namespace bfc {
  constexpr int64_t InvalidIndex = std::numeric_limits<int64_t>::max();

  enum OBJKeyword {
    OBJKeyword_Object,        // unsupported
    OBJKeyword_Group,         // unsupported
    OBJKeyword_SmoothShading, // unsupported
    OBJKeyword_Vertex,
    OBJKeyword_Normal,
    OBJKeyword_TexCoord,
    OBJKeyword_Line,
    OBJKeyword_Face,
    OBJKeyword_MatLib,
    OBJKeyword_MatRef,
    OBJKeyword_Comment,
    OBJKeyword_None
  };

  static OBJKeyword ScanKeyword(StringView const & str) {
    if (str.equals("v", true))
      return OBJKeyword_Vertex;
    else if (str.equals("vn", true))
      return OBJKeyword_Normal;
    else if (str.equals("vt", true))
      return OBJKeyword_TexCoord;
    else if (str.equals("mtllib", true))
      return OBJKeyword_MatLib;
    else if (str.equals("usemtl", true))
      return OBJKeyword_MatRef;
    else if (str.equals("s", true))
      return OBJKeyword_SmoothShading;
    else if (str.equals("f", true))
      return OBJKeyword_Face;
    else if (str.equals("l", true))
      return OBJKeyword_Line;
    else if (str.equals("#", true))
      return OBJKeyword_Comment;
    return OBJKeyword_None;
  }

  class BufferedReader : public Stream {
  public:
    BufferedReader(Stream * pStream, int64_t chunkSize = 32 * 1024 * 1024ll)
      : m_pStream(pStream)
      , m_bufferedSize(chunkSize)
      , m_chunkStart(pStream->tell())
      , m_chunkPos(0)
    {}

    virtual bool readable() const {
      return true;
    }

    virtual bool writeable() const {
      return false;
    }

    virtual bool seekable() const {
      return false;
    }

    virtual bool eof() const {
      return m_chunkStart + m_chunkPos == m_pStream->tell() && m_pStream->eof();
    }

    virtual int64_t write(void const* data, int64_t length) {
      BFC_UNUSED(data, length);
      return 0;
    }

    virtual int64_t read(void* data, int64_t length) {
      int64_t remaining = length;

      while (remaining > 0) {
        const int64_t available = m_chunkSize - m_chunkPos;
        const int64_t readSize  = math::min(length, available);
        memcpy(data, m_chunk.begin() + m_chunkPos, readSize);

        remaining  -= readSize;
        m_chunkPos += readSize;

        if (m_chunkPos == m_chunkSize) {
          m_chunkStart = m_pStream->tell();
          m_chunkPos   = 0;

          m_chunk.reserve(m_bufferedSize);
          m_chunkSize = m_pStream->read(m_chunk.begin(), m_bufferedSize);

          if (m_chunk.size() == 0) {
            break;
          }
        }
      }

      return length - remaining;
    }

    virtual int64_t tell() const {
      return m_chunkStart + m_chunkPos;
    }

  private:
    int64_t m_bufferedSize = 0;
    int64_t m_chunkStart   = 0;
    int64_t m_chunkPos     = 0;

    Stream *        m_pStream = nullptr;
    Vector<uint8_t> m_chunk;
    int64_t         m_chunkSize = 0;
  };

  bool OBJParser::read(Stream * pStream, MeshData * pMesh, StringView const & resourceDir) {
    BufferedReader bufferedReader(pStream); // Buffered because we are often reading a large amount of data
    TextReader     reader(&bufferedReader);

    // Reserve memory based on file size to reduce allocations
    // pMesh->normals.reserve(data.size() / 60);
    // pMesh->triangles.reserve(data.size() / 180);
    // pMesh->positions.reserve(data.size() / 60);
    // pMesh->uvs.reserve(data.size() / 60);

    String               mtlFile = "";
    String               curMat  = "";
    Map<String, int64_t> matNames;
    int64_t              matID = 0;

    Vector<StringView> delims = {" ", "\t", "\n"};
    StringSplitter     splitter;
    StringSplitter     vertDefSplitter;
    while (!bufferedReader.eof()) {
      StringView line = reader.readLine();
      splitter(line, delims, true);
      if (splitter.tokens.size() == 0)
        continue;

      Vector<StringView> & tokens = splitter.tokens;

      switch (ScanKeyword(tokens.front())) {
      case OBJKeyword_Face: // Scan face definition and triangulate
      {
        int64_t firstVert  = -1;
        int64_t lastVert   = -1;
        bool    isFirstTri = true;
        for (int64_t v = 1; v < tokens.size();) {
          pMesh->triangles.pushBack(MeshData::Triangle{});
          MeshData::Triangle & tri = pMesh->triangles.back();
          tri.material             = matID;
          if (!isFirstTri) {
            tri.vertex[0] = firstVert;
            tri.vertex[1] = lastVert;
          }

          for (int64_t v2 = isFirstTri ? 0 : 2; v < tokens.size() && v2 < 3; ++v2, ++v) {
            int64_t                    len        = 0;
            int64_t                    slashIndex = 0;
            MeshData::Vertex           vert;
            int64_t *                  mapping[] = {&vert.position, &vert.uv, &vert.normal};
            Vector<StringView> const & indices   = vertDefSplitter(tokens[v], "/", false);
            for (int64_t i = 0; i < indices.size(); ++i) {
              if (indices[i].length() > 0) {
                *mapping[i] = Scan::readInt(indices[i]);
              } else {
                *mapping[i] = 0; // Will be mapped to -1 (invalid) later
              }
            }

            tri.vertex[v2] = pMesh->vertices.size();
            pMesh->vertices.pushBack(vert);
          }

          if (isFirstTri)
            firstVert = tri.vertex[0];
          lastVert = tri.vertex[2];

          isFirstTri = false;
        }
      } break;
      case OBJKeyword_Vertex:
        if (tokens.size() >= 4) {
          pMesh->positions.pushBack({Scan::readFloat(tokens[1]), Scan::readFloat(tokens[2]), Scan::readFloat(tokens[3])});
        } else {
          pMesh->positions.pushBack(Vec3d(0));
        }

        break;
      case OBJKeyword_Normal:
        if (tokens.size() >= 4) {
          pMesh->normals.pushBack({Scan::readFloat(tokens[1]), Scan::readFloat(tokens[2]), Scan::readFloat(tokens[3])});
        } else {
          pMesh->normals.pushBack(Vec3d(0));
        }

        break;
      case OBJKeyword_TexCoord:
        if (tokens.size() >= 3) {
          pMesh->uvs.pushBack({Scan::readFloat(tokens[1]), Scan::readFloat(tokens[2])});
        } else {
          pMesh->uvs.pushBack(Vec3d(0));
        }

        break;
      case OBJKeyword_MatLib: mtlFile = line.substr(tokens[1].begin() - line.begin()).trim(); break;
      case OBJKeyword_MatRef:
        curMat = line.substr(tokens[1].begin() - line.begin()).trim();
        matID  = matNames.size();
        if (!matNames.tryAdd(curMat, matID))
          matID = matNames[curMat];
        break;
      case OBJKeyword_Object:
      case OBJKeyword_Group:
      case OBJKeyword_Line:
      case OBJKeyword_None:
      case OBJKeyword_Comment: break;
      }
    }

    // Parse negative indices
    for (MeshData::Vertex & vert : pMesh->vertices) {
      vert.position = vert.position >= 0 ? vert.position - 1 : pMesh->positions.size() + vert.position;
      vert.uv       = vert.uv >= 0 ? vert.uv - 1 : pMesh->uvs.size() + vert.uv;
      vert.normal   = vert.normal >= 0 ? vert.normal - 1 : pMesh->normals.size() + vert.normal;
      vert.colour   = vert.colour >= 0 ? vert.colour - 1 : pMesh->colours.size() + vert.colour;
    }

    Vector<String> materialOrder(matNames.size(), "");
    for (auto & [name, index] : matNames)
      materialOrder[index] = name;

    File   f;
    String fullPath = resourceDir;
    fullPath        = fullPath.concat("/").concat(mtlFile);
    if (f.open(fullPath)) {
      pMesh->materials = MTLParser::read(&f, materialOrder);
    } else {
      pMesh->materials.resize(materialOrder.size());
      for (auto & [i, name] : enumerate(materialOrder)) {
        pMesh->materials[i].setName(name);
      }
    }
    return true;
  }

  bool OBJParser::write(Stream * pStream, MeshData const * pMesh) {
    BFC_RELASSERT(false, "Not implemented");
    return false;
  }

  Vector<MeshData::Material> MTLParser::read(Stream * pStream, Span<String> const & names) {
    TextReader reader(pStream);

    int64_t                    lineNum = 0;
    StringSplitter             splitter;
    Vector<MeshData::Material> materials;
    MeshData::Material *       pCurMat = nullptr;

    while (!pStream->eof()) {
      StringView line = reader.readLine().trim();

      splitter(line, " ", true);
      if (splitter.tokens.size() == 0)
        continue;

      Vector<StringView> & tokens = splitter.tokens;

      auto ReadValue = [&](StringView const & propertyName) {
        if (tokens.size() != 2) {
          return false;
        }

        pCurMat->setValue(propertyName, Scan::readFloat(tokens[1]));
        return true;
      };

      auto ReadColour = [&](StringView const & propertyName) {
        Vec4d colour;
        colour.x = tokens.size() >= 2 ? Scan::readFloat(tokens[1]) : 0;
        colour.y = tokens.size() >= 3 ? Scan::readFloat(tokens[2]) : 0;
        colour.z = tokens.size() >= 4 ? Scan::readFloat(tokens[3]) : 0;
        colour.w = tokens.size() >= 5 ? Scan::readFloat(tokens[4]) : 1;

        pCurMat->setColour(propertyName, colour);
        return true;
      };

      auto ReadTexture = [&](StringView const & propertyName) {
        if (tokens.size() <= 1) {
          return false;
        }

        StringView texPath = StringView(tokens[1].begin(), line.end() - tokens[1].begin());
        if (texPath.length() == 0) {
          return false;
        }

        pCurMat->setTexture(propertyName, texPath);
        return true;
      };

      StringView prop = tokens[0];

      // Ignore comments
      if (prop == "#") {
        continue;
      }

      if (tokens.size() < 2) {
        continue;
      }

      if (prop.equals("newmtl", true)) {
        StringView matName = line.substr(tokens[1].begin() - line.begin()).trim();

        if (matName.length() == 0) {
          printf("ln %lld: Cannot add new material. No name specified.\n", lineNum);
        }

        pCurMat = nullptr;

        for (int64_t i = 0; i < materials.size(); ++i) {
          if (materials[i].getName() == matName) {
            pCurMat = materials.begin() + i;
            break;
          }
        }

        if (pCurMat == nullptr) {
          materials.pushBack(MeshData::Material());
          materials.back().setName(matName);
          pCurMat = &materials.back();
        }

        continue;
      }

      if (pCurMat == nullptr) {
        printf("ln %lld: Cannot read material property ('%*.s'). No active material.\n", lineNum, (int)prop.length(), prop.begin());
        continue;
      }

      // Properties
      if (prop.equals("Ka", true)) {
        ReadColour(MeshData::Material::Phong::ambient);
      } else if (prop.equals("Kd", true)) {
        ReadColour(MeshData::Material::Phong::diffuse);
      } else if (prop.equals("Ks", true)) {
        ReadColour(MeshData::Material::Phong::specular);
      } else if (prop.equals("d", true)) {
        ReadValue(MeshData::Material::Phong::alpha);
      } else if (prop.equals("Ns", true)) {
        ReadValue(MeshData::Material::Phong::specularPower);
      } else if (prop.equals("Tr", true)) {
        ReadValue(MeshData::Material::Phong::alpha);
        pCurMat->setValue(MeshData::Material::Phong::alpha, 1 - pCurMat->getValue(MeshData::Material::Phong::alpha));
      } else if (prop.equals("Ni", true)) {
        ReadValue("ior");
      } else if (prop.equals("Pr", true)) {
        ReadValue(MeshData::Material::PBR::roughness);
      } else if (prop.equals("Pm", true)) {
        ReadValue(MeshData::Material::PBR::metalness);
      } else if (prop.equals("Ps", true)) {
        ReadValue("sheen");
      } else if (prop.equals("Ke", true)) {
        ReadColour(MeshData::Material::PBR::emissive);
      }
      // Maps
      else if (prop.equals("map_Ka", true)) {
        ReadTexture(MeshData::Material::Phong::ambient);
      } else if (prop.equals("map_Kd", true)) {
        ReadTexture(MeshData::Material::Phong::diffuse);
      } else if (prop.equals("map_Ks", true)) {
        ReadTexture(MeshData::Material::Phong::specular);
      } else if (prop.equals("map_d", true)) {
        ReadTexture(MeshData::Material::Phong::alpha);
      } else if (prop.equals("map_bump", true) || prop.equals("bump", true)) {
        ReadTexture(MeshData::Material::Phong::normal);
      } else if (prop.equals("map_Disp", true) || prop.equals("disp", true)) {
        ReadTexture(MeshData::Material::PBR::normal);
      } else if (prop.equals("norm", true)) {
        ReadTexture(MeshData::Material::PBR::normal);
      } else if (prop.equals("map_Pr", true)) {
        ReadTexture(MeshData::Material::PBR::roughness);
      } else if (prop.equals("map_Pm", true)) {
        ReadTexture(MeshData::Material::PBR::metalness);
      } else if (prop.equals("map_Ps", true)) {
        ReadTexture("sheen");
      } else if (prop.equals("map_Ke", true)) {
        ReadTexture(MeshData::Material::PBR::emissive);
      }
      // else if (prop.compare("map_RMA", atSCO_None)) {
      //   ReadTexture(line, "roughness");
      //   ReadTexture(line, "metalness");
      //   ReadTexture(line, "ambient-occlusion");
      // }
      else {
        printf("ln %lld: Unsupported material property '%s'.\n", lineNum, String(prop).c_str());
      }
    }

    if (names.size() > 0) {
      Vector<MeshData::Material> mats;
      for (StringView const & name : names) {
        for (MeshData::Material const & m : materials) {
          if (m.getName() == name) {
            mats.pushBack(m);
            break;
          }
        }
      }

      return mats;
    }

    return materials;
  }

  bool MTLParser::write(Stream * pStream, Vector<MeshData::Material> const & materials) {
    BFC_RELASSERT(false, "Not implemented");
    return false;
  }
} // namespace bfc
