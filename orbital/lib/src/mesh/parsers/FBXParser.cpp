#include "mesh/parsers/FBXParser.h"

#ifdef BFC_ENABLE_FBX_SDK
#include "core/Set.h"
#include "core/Stream.h"

#define FBXSDK_SHARED
#include "fbxsdk.h"

namespace bfc {
  class FbxStreamAdapter : public FbxStream {
  public:
    FbxStreamAdapter(Stream * pStream, int readerID, int writerID)
      : m_pStream(pStream)
      , m_readerID(readerID)
      , m_writerID(writerID) {}

    virtual EState GetState() {
      return EState::eOpen;
    }

    virtual bool Open(void * pStreamData) {
      return m_pStream->seek(0, SeekOrigin_Start);
    }

    virtual bool Close() {
      return true;
    }

    virtual bool Flush() {
      return m_pStream->flush();
    }

    virtual int Write(const void * pData, int size) {
      return (int)m_pStream->write(pData, size);
    }

    virtual int Read(void * pData, int size) const {
      return (int)m_pStream->read(pData, size);
    }

    virtual int GetReaderID() const {
      return m_readerID;
    }

    virtual int GetWriterID() const {
      return m_writerID;
    }

    virtual void Seek(const FbxInt64 & pOffset, const FbxFile::ESeekPos & pSeekPos) {
      SeekOrigin so = SeekOrigin_Current;
      switch (pSeekPos) {
      case FbxFile::eBegin: so = SeekOrigin_Start; break;
      case FbxFile::eCurrent: so = SeekOrigin_Current; break;
      case FbxFile::eEnd: so = SeekOrigin_End; break;
      }

      if (!m_pStream->seek(pOffset, so))
        m_error = 1;
    }

    virtual long GetPosition() const {
      return (long)m_pStream->tell();
    }

    virtual void SetPosition(long pPosition) {
      if (!m_pStream->seek(pPosition, SeekOrigin_Start))
        m_error = 1;
    }

    virtual int GetError() const {
      return m_error;
    }

    virtual void ClearError() {
      m_error = 0;
    }

  private:
    Stream * m_pStream  = nullptr;
    int      m_error    = 0;
    int      m_readerID = -1;
    int      m_writerID = -1;
  };

  template<typename DstVec, typename SrcVec>
  void copyElementArray(Vector<DstVec> * pDst, const FbxLayerElementArrayTemplate<SrcVec> & src) {
    using DstElemT         = std::decay_t<decltype(std::declval<DstVec>()[0])>;
    using SrcElemT         = std::decay_t<decltype(std::declval<SrcVec>()[0])>;
    constexpr int dstWidth = sizeof(DstVec) / sizeof(DstElemT);
    constexpr int srcWidth = sizeof(SrcVec) / sizeof(SrcElemT);
    constexpr int width    = math::min(dstWidth, srcWidth);

    pDst->clear();
    pDst->reserve(src.GetCount());

    for (int i = 0; i < src.GetCount(); ++i) {
      DstVec v;
      for (int j = 0; j < width; ++j) {
        ((DstElemT *)&v)[j] = (DstElemT)((const SrcElemT *)&src[i])[j];
      }
      pDst->pushBack(v);
    }
  }

  template<typename T>
  int getElementIndex(FbxLayerElementTemplate<T> * pElement, int polyIndex, int polyVertIndex, int controlPointIndex) {
    if (pElement == nullptr) {
      return -1;
    }

    int mapped = 0;
    switch (pElement->GetMappingMode()) {
    case FbxGeometryElement::eByControlPoint: mapped = controlPointIndex; break;
    case FbxGeometryElement::eByPolygon: mapped = polyIndex; break;
    case FbxGeometryElement::eByPolygonVertex: mapped = polyVertIndex; break;
    case FbxGeometryElement::eAllSame: mapped = 0; break;
    default: return -1;
    }

    if (pElement->GetReferenceMode() == FbxGeometryElement::eIndexToDirect || pElement->GetReferenceMode() == FbxGeometryElement::eIndex)
      return pElement->GetIndexArray()[mapped];

    return mapped;
  }

  MeshData convertMesh(FbxMesh * pFbxMesh) {
    FbxGeometryConverter converter(pFbxMesh->GetFbxManager());

    if (!pFbxMesh->IsTriangleMesh()) {
      pFbxMesh = (FbxMesh *)converter.Triangulate(pFbxMesh, true);
    }

    MeshData     ret;
    int          numControlPoints = pFbxMesh->GetControlPointsCount();
    FbxVector4 * pControlPoints   = pFbxMesh->GetControlPoints();

    ret.positions.reserve(numControlPoints);
    for (int i = 0; i < numControlPoints; ++i) {
      const FbxVector4 & point = pControlPoints[i];
      ret.positions.pushBack(Vec3d(point[0], point[1], point[2]));
    }

    FbxGeometryElementNormal *      pNormals  = pFbxMesh->GetElementNormal(0);
    FbxGeometryElementTangent *     pTangents = pFbxMesh->GetElementTangent(0);
    FbxGeometryElementVertexColor * pColors   = pFbxMesh->GetElementVertexColor();
    // FbxGeometryElementBinormal * pBinormals = pFbxMesh->GetElementBinormal(0);
    FbxGeometryElementUV *       pUVs       = pFbxMesh->GetElementUV(0);
    FbxGeometryElementMaterial * pMaterials = pFbxMesh->GetElementMaterial(0);

    if (pNormals != nullptr)
      copyElementArray(&ret.normals, pNormals->GetDirectArray());
    if (pTangents != nullptr)
      copyElementArray(&ret.tangents, pTangents->GetDirectArray());
    if (pUVs != nullptr)
      copyElementArray(&ret.uvs, pUVs->GetDirectArray());
    if (pColors != nullptr)
      copyElementArray(&ret.colours, pColors->GetDirectArray());

    int   numPolygons      = pFbxMesh->GetPolygonCount();
    int * pPolygonVertices = pFbxMesh->GetPolygonVertices();

    for (int polyIndex = 0; polyIndex < numPolygons; ++polyIndex) {
      int                startIndex = pFbxMesh->GetPolygonVertexIndex(polyIndex);
      MeshData::Triangle tri;
      for (int vert = 0; vert < 3; ++vert) {
        int polyVert     = startIndex + vert;
        int controlPoint = pPolygonVertices[polyVert];

        MeshData::Vertex v;
        v.position = controlPoint;
        v.colour   = getElementIndex(pColors, polyIndex, polyVert, controlPoint);
        v.normal   = getElementIndex(pNormals, polyIndex, polyVert, controlPoint);
        v.tangent  = getElementIndex(pTangents, polyIndex, polyVert, controlPoint);
        v.uv       = getElementIndex(pUVs, polyIndex, polyVert, controlPoint);

        tri.vertex[vert] = ret.vertices.size();
        ret.vertices.pushBack(v);
      }

      tri.material = getElementIndex(pMaterials, polyIndex, startIndex, pPolygonVertices[startIndex]);
      ret.triangles.pushBack(tri);
    }

    return ret;
  }

  Map<FbxTexture *, Filename> buildTextureCache(FbxScene * pFbxScene) {
    Map<FbxTexture *, Filename> ret;
    FbxArray<FbxTexture *>      textures;
    pFbxScene->FillTextureArray(textures);
    for (auto & [i, pTexture] : enumerate(textures.GetArray(), textures.GetArray() + textures.GetCount())) {
      FbxFileTexture * pFileTexture = FbxCast<FbxFileTexture>(pTexture);

      if (pFileTexture == nullptr) {
        continue;
      }

      ret.add(pTexture, pFileTexture->GetRelativeFileName());
    }

    return ret;
  }

  MeshData::Material convertMaterial(FbxSurfaceMaterial const * pSrcMaterial, Map<FbxTexture *, Filename> const & textureCache) {
    static Map<String, String> valueMapping = {
      {FbxSurfaceMaterial::sAmbientFactor, MeshData::Material::Phong::ambient},
      {FbxSurfaceMaterial::sDiffuseFactor, MeshData::Material::Phong::diffuse},
      {FbxSurfaceMaterial::sSpecularFactor, MeshData::Material::Phong::specular},
      {FbxSurfaceMaterial::sEmissiveFactor, MeshData::Material::PBR::emissive},
    };

    static Map<String, String> colorMapping = {
      {FbxSurfaceMaterial::sAmbient, MeshData::Material::Phong::ambient},
      {FbxSurfaceMaterial::sDiffuse, MeshData::Material::Phong::diffuse},
      {FbxSurfaceMaterial::sSpecular, MeshData::Material::Phong::specular},
      {FbxSurfaceMaterial::sEmissive, MeshData::Material::PBR::emissive},
    };

    MeshData::Material ret;

    // Read values
    for (auto & [fbxProperty, name] : valueMapping) {
      const FbxProperty factor = pSrcMaterial->FindProperty(fbxProperty.c_str());
      if (factor.IsValid()) {
        ret.setValue(name, factor.Get<FbxDouble>());
      }
    }

    // Read colours/textures
    for (auto & [fbxProperty, name] : colorMapping) {
      const FbxProperty value = pSrcMaterial->FindProperty(fbxProperty.c_str());
      if (!value.IsValid())
        continue;

      Vec4 colour = Vec4((const Vec3d &)value.Get<FbxDouble3>(), 1.0);
      ret.setColour(name, colour);

      if (value.GetSrcObjectCount<FbxTexture>() > 0) {
        FbxTexture * pTexture = value.GetSrcObject<FbxTexture>();
        Filename     path;
        if (textureCache.tryGet(pTexture, &path)) {
          ret.setTexture(name, path.getView());
        }
      }
    }

    return ret;
  }

  Map<FbxSurfaceMaterial *, int64_t> buildMaterialCache(FbxScene * pFbxScene, Vector<MeshData::Material> * pMaterialStorage,
                                                        Map<FbxTexture *, Filename> const & textureCache) {
    Map<FbxSurfaceMaterial *, int64_t> ret;
    FbxArray<FbxSurfaceMaterial *>     materials;
    pFbxScene->FillMaterialArray(materials);
    for (auto & [i, pMaterial] : enumerate(materials.GetArray(), materials.GetArray() + materials.GetCount()))
      if (ret.tryAdd(pMaterial, pMaterialStorage->size()))
        pMaterialStorage->pushBack(convertMaterial(pMaterial, textureCache));
    return ret;
  }

  Map<FbxMesh *, int64_t> buildMeshCache(Set<FbxMesh *> const & meshes, Vector<MeshData> * pMeshStorage) {
    Map<FbxMesh *, int64_t> ret;
    for (FbxMesh * pFbxMesh : meshes)
      if (ret.tryAdd(pFbxMesh, pMeshStorage->size()))
        pMeshStorage->pushBack(convertMesh(pFbxMesh));
    return ret;
  }

  void addSkeletonNodeRecursive(FbxSkeleton * pRoot, int64_t parentID, int64_t skeletonID, MeshSkeleton * pSkeleton,
                                Map<FbxSkeleton *, Pair<int64_t, int64_t>> * pCache) {
    FbxNode * pNode       = pRoot->GetNode();
    int       numChildren = pNode->GetChildCount();

    for (int i = 0; i < numChildren; ++i) {
      FbxNode *     pChildNode = pNode->GetChild(i);
      FbxSkeleton * pChildSkeleton  = pChildNode->GetSkeleton();

      if (pChildSkeleton == nullptr) {
        continue;
      }

      int64_t newBoneID = pSkeleton->addBone(pChildSkeleton->GetName(), parentID);
      pCache->add(pChildSkeleton, Pair(skeletonID, newBoneID));
      addSkeletonNodeRecursive(pChildSkeleton, newBoneID, skeletonID, pSkeleton, pCache);
    }
  }

  MeshSkeleton parseSkeleton(FbxSkeleton * pSkeleton, int64_t skeletonID, Map<FbxSkeleton *, Pair<int64_t, int64_t>> * pCache) {
    pCache->add(pSkeleton, { skeletonID, 0 });

    MeshSkeleton ret;
    ret.setBoneName(0, pSkeleton->GetName());

    addSkeletonNodeRecursive(pSkeleton, 0, skeletonID, &ret, pCache);

    return ret;
  }

  Map<FbxSkeleton *, Pair<int64_t, int64_t>> buildBoneCache(Set<FbxSkeleton *> const & skeletons, Vector<MeshSkeleton> * pSkeletonStorage) {
    Map<FbxSkeleton *, Pair<int64_t, int64_t>> ret;

    for (FbxSkeleton * pSkeleton : skeletons) {
      if (!pSkeleton->IsSkeletonRoot()) {
        continue;
      }

      MeshSkeleton skeleton = parseSkeleton(pSkeleton, pSkeletonStorage->size(), &ret);
      pSkeletonStorage->pushBack(skeleton);
    }

    return ret;
  }

  bool readScene(FbxScene * pFbxScene, FBXParser::Scene * pScene, StringView const & resourceDir) {
    Set<FbxMesh *> meshes;
    Set<FbxSkeleton *> bones;

    Map<FbxNode *, int64_t> nodeLookup;
    int                     numNodes = pFbxScene->GetNodeCount();
    pScene->nodes.resize(numNodes);
    for (int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex) {
      FbxNode * pNode = pFbxScene->GetNode(nodeIndex);
      nodeLookup.add(pNode, nodeIndex);

      int numAttributes = pNode->GetNodeAttributeCount();
      for (int attributeIndex = 0; attributeIndex < numAttributes; ++attributeIndex) {
        FbxNodeAttribute * pAttribute = pNode->GetNodeAttributeByIndex(attributeIndex);

        switch (pAttribute->GetAttributeType()) {
        case FbxNodeAttribute::eMesh: meshes.add((FbxMesh *)pAttribute); break;
        case FbxNodeAttribute::eSkeleton: bones.add((FbxSkeleton *)pAttribute); break;
        }
      }
    }

    Map<FbxTexture *, Filename>                textures       = buildTextureCache(pFbxScene);
    Map<FbxMesh *, int64_t>                    meshCache      = buildMeshCache(meshes, &pScene->meshes);
    Map<FbxSkeleton *, Pair<int64_t, int64_t>> boneCache      = buildBoneCache(bones, &pScene->skeletons);
    Map<FbxSurfaceMaterial *, int64_t>         materialLookup = buildMaterialCache(pFbxScene, &pScene->materials, textures);
      
    for (int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex) {
      FbxNode *         pFbxNode = pFbxScene->GetNode(nodeIndex);
      FBXParser::Node & node     = pScene->nodes[nodeIndex];

      node.parentNode = nodeLookup.getOr(pFbxNode->GetParent(), -1);
      for (int materialIndex = 0; materialIndex < pFbxNode->GetMaterialCount(); ++materialIndex) {
        node.materials.pushBack(materialLookup.getOr(pFbxNode->GetMaterial(materialIndex), -1));
      }

      node.globalTransform = (Mat4d const &)pFbxNode->EvaluateGlobalTransform();
      node.transform = (Mat4d const &)pFbxNode->EvaluateLocalTransform();

      int numAttributes = pFbxNode->GetNodeAttributeCount();
      for (int attributeIndex = 0; attributeIndex < numAttributes; ++attributeIndex) {
        FbxNodeAttribute * pAttribute = pFbxNode->GetNodeAttributeByIndex(attributeIndex);

        switch (pAttribute->GetAttributeType()) {
        case FbxNodeAttribute::eMesh: node.mesh = meshCache.getOr((FbxMesh *)pAttribute, -1); break;
        }
      }
    }

    for (int poseIndex = 0; poseIndex < pFbxScene->GetPoseCount(); ++poseIndex) {
      FbxPose * pFbxPose = pFbxScene->GetPose(poseIndex);
      int64_t skeletonIndex = -1;
      int64_t poseID   = -1;
      int numNodes = pFbxPose->GetCount();

      for (int nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex) {
        FbxNode *              pNode = pFbxPose->GetNode(nodeIndex);
        Pair<int64_t, int64_t> mappedBone;
        if (!boneCache.tryGet(pNode->GetSkeleton(), &mappedBone)) {
          continue;
        }

        if (skeletonIndex == -1) {
          skeletonIndex = mappedBone.first;
        }

        MeshSkeleton & skeleton = pScene->skeletons[skeletonIndex];
        if (poseID == -1) {
          poseID = skeleton.addPose(pFbxPose->GetName());
        }

        Mat4d nodeTransform = (Mat4d const &)pFbxPose->GetMatrix(nodeIndex);
        if (pFbxPose->IsLocalMatrix(nodeIndex)) {
          skeleton.setBoneLocalTransform(poseID, mappedBone.second, nodeTransform);
        } else {
          skeleton.setBoneGlobalTransform(poseID, mappedBone.second, nodeTransform);
        }
      }
    }

    return true;
  }

  bool FBXParser::read(Stream * pStream, Scene * pScene) {
    FbxManager * pSdkManager = FbxManager::Create();

    int readerID = pSdkManager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX (*.fbx)");
    int writerID = pSdkManager->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)");

    FbxStreamAdapter fbxStream(pStream, readerID, writerID);
    FbxIOSettings *  pSettings = FbxIOSettings::Create(pSdkManager, IOSROOT);
    pSdkManager->SetIOSettings(pSettings);

    FbxImporter * pImporter = FbxImporter::Create(pSdkManager, "");

    FbxScene * pFbxScene = FbxScene::Create(pSdkManager, "");
    bool       success   = pImporter->Initialize(&fbxStream, 0, -1, pSdkManager->GetIOSettings());
    success              = success && pImporter->Import(pFbxScene);
    success              = success && readScene(pFbxScene, pScene, "");

    pFbxScene->Destroy();
    pImporter->Destroy();
    pSettings->Destroy();
    pSdkManager->Destroy();

    return success;
  }

  bool FBXParser::read(Stream * pStream, MeshData * pMesh, StringView const & resourceDir) {
    Scene scene;
    if (!read(pStream, &scene))
      return false;
    
    *pMesh = MeshData();

    for (Node& node : scene.nodes) {
      if (node.mesh == -1)
        continue;

      MeshData subMesh = scene.meshes[node.mesh];
      subMesh.applyTransform(node.globalTransform);
      for (int64_t mat : node.materials) {
        subMesh.materials.pushBack(scene.materials[mat]);
      }

      pMesh->merge(subMesh);
    }

    return true;
  }

  bool FBXParser::write(Stream * pStream, MeshData const * pMesh) {
    return false;
  }
} // namespace bfc

#else

namespace bfc {
  bool FBXParser::read(Stream * pStream, Scene * pScene) {
    BFC_UNUSED(pStream, pScene);
    return false;
  }

  bool FBXParser::read(Stream * pStream, MeshData * pMesh, StringView const & resourceDir) {
    BFC_UNUSED(pStream, pMesh, resourceDir);
    return false;
  }

  bool FBXParser::write(Stream * pStream, MeshData const * pMesh) {
    BFC_UNUSED(pStream, pMesh);
    return false;
  }
} // namespace bfc

#endif
