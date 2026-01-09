#pragma once

#include "../core/Vector.h"
#include "../core/Map.h"
#include "../core/URI.h"
#include "../core/Filename.h"
#include "../math/MathTypes.h"

namespace bfc {
  template<typename T>
  struct Serializer;
  class SerializedObject;

  enum MeshDeformerMode {
    MeshDeformerMode_Normalize,
    MeshDeformerMode_Additive,
    MeshDeformerMode_TotalOne,
  };

  class BFC_API MeshData {
  public:
    class BFC_API Material {
      friend Serializer<bfc::MeshData::Material>;
    public:
      using PropertyID = Pair<String, int64_t>;

      void setName(StringView name);

      void setColour(StringView name, Vec4 colour, int64_t layer = 0);

      void setValue(StringView name, double value, int64_t layer = 0);

      void setTexture(StringView name, StringView texture, int64_t layer = 0);

      StringView getName() const;

      Vec4 getColour(StringView name, int64_t layer = 0, Vec4 defaultValue = Vec4(0)) const;

      double getValue(StringView name, int64_t layer = 0, double defaltValue = 0.0) const;

      StringView getTexture(StringView name, int64_t layer = 0) const;

      bool hasColour(StringView name, int64_t layer = 0) const;

      bool hasValue(StringView name, int64_t layer = 0) const;

      bool hasTexture(StringView name, int64_t layer = 0) const;

      Vector<PropertyID> getTextures() const;

      Vector<PropertyID> getValues() const;

      Vector<PropertyID> getColours() const;

      bool read(URI const & uri);

      bool write(URI const & uri);

      struct BFC_API Phong {
        static const StringView ambient;
        static const StringView diffuse;
        static const StringView specular;
        static const StringView alpha;
        static const StringView specularPower;
        static const StringView normal;
      };

      struct BFC_API PBR {
        static const StringView albedo;
        static const StringView ambient;
        static const StringView emissive;
        static const StringView roughness;
        static const StringView metalness;
        static const StringView normal;
        static const StringView alpha;
        static const StringView ao;
      };

      BFC_API friend int64_t write(Stream * pStream, MeshData::Material const * pValue, int64_t count);
      BFC_API friend int64_t read(Stream * pStream, MeshData::Material * pValue, int64_t count);

    private:
      String                  m_name;
      Map<PropertyID, Vec4>   m_colours;
      Map<PropertyID, double> m_values;
      Map<PropertyID, String> m_textures;
    };

    MeshData() = default;

    MeshData(URI const & uri);

    static bool canRead(StringView const & extension);

    bool read(URI const & uri);
    bool write(URI const & uri);

    void validate();
    void addDefaults(bool validateFirst = true);
    void findTextures();
    void mapTextures(std::function<Filename(Filename const &path, bfc::String const &name, int64_t layer)> const & func);

    // Mesh triangles must have positions and uvs.
    void calculateTangents();
    void calculateFlatNormals();
    // void calculateSmoothNormals(float cornerThresold);

    void merge(MeshData const &m);

    void applyTransform(Mat4d const & transform);
    void transformPositions(Mat4d const & transform);
    void transformNormals(Mat4d const & transform);

    struct Vertex {
      int64_t position = -1;
      int64_t uv = -1;
      int64_t colour = -1;
      int64_t normal = -1;
      int64_t tangent = -1;
    };

    struct Triangle {
      int64_t vertex[3];
      int64_t material = -1;
    };

    struct Deformer {
      struct Weight {
        int64_t vertex = -1;
        float   weight = 1;
      };

      String           name;     ///< Name of this deformer
      MeshDeformerMode mode;     ///< Mode that the weights influence geometry
      Vector<Weight>   vertices; ///< Vertices influenced by this deformer
    };

    struct Skin {
      String           name;      ///< Name of this skin
      Vector<Deformer> deformers; ///< Deformers in this skin
    };

    Vector<Vec3d> positions;
    Vector<Vec2d> uvs;
    Vector<Vec4d> colours;
    Vector<Vec3d> normals;
    Vector<Vec3d> tangents;

    Vector<Vertex> vertices;
    Vector<Triangle> triangles;
    Vector<Material> materials;

    Vector<Skin> skins;

    Filename sourceFile;
  };

  BFC_API int64_t write(Stream * pStream, MeshData const * pValue, int64_t count);
  BFC_API int64_t read(Stream * pStream, MeshData * pValue, int64_t count);

  template<>
  struct Serializer<bfc::MeshData::Material> {
    template<typename Context>
    static SerializedObject write(bfc::MeshData::Material const & o, Context const & ctx) {
      return SerializedObject::MakeMap({
        { "name", serialize(o.getName(), ctx) },
        { "textures", serialize(o.m_textures, ctx) },
        { "colours", serialize(o.m_colours, ctx) },
        { "values", serialize(o.m_values, ctx) },
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, bfc::MeshData::Material & o, Context const & ctx) {
      s.get("name").readOrConstruct(o.m_name);
      s.get("textures").readOrConstruct(o.m_textures);
      s.get("colours").readOrConstruct(o.m_colours);
      s.get("values").readOrConstruct(o.m_values);
      return true;
    }
  };
}
