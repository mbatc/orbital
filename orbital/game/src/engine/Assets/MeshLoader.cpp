#include "MeshLoader.h"
#include "AssetManager.h"
#include "AssetLoadContext.h"

#include "mesh/Mesh.h"
#include "mesh/parsers/OBJParser.h"
#include "mesh/parsers/FBXParser.h"

#include "util/Scan.h"

using namespace bfc;

namespace engine {
  Ref<MeshData> MeshDataFileLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    BFC_UNUSED(pContext);

    Ref<Stream> pStream = pContext->getFileSystem()->open(uri, FileMode_ReadBinary);
    if (pStream == nullptr) {
      return nullptr;
    }

    Ref<MeshData> pMeshData = NewRef<MeshData>();

    bool       success    = false;
    StringView ext        = Filename::extension(uri.pathView());
    Filename   parentPath = Filename::parent(pContext->getFileSystem()->resolveUri(uri).path());

    if (ext.equals("obj", true)) {
      // TODO: more customization of how dependent files are loaded.
      success = OBJParser::read(pStream.get(), pMeshData.get(), parentPath);
    } else if (ext.equals("fbx", true)) {
      success = FBXParser::read(pStream.get(), pMeshData.get(), parentPath);
    }

    if (!success) {
      return nullptr;
    }

    pMeshData->sourceFile = uri.path();
    pMeshData->addDefaults();

    pMeshData->mapTextures([=](Filename const & file, String const & /*name*/, int64_t /*layer*/) {
      return pContext->getFileSystem()->find(URI::File(file), { uri.resolveRelativeReference("../") }).path();
    });

    pMeshData->findTextures();

    return pMeshData;
  }

  bool MeshDataFileLoader::handles(URI const & uri, AssetManager const * pManager) const {
    BFC_UNUSED(pManager);

    return MeshData::canRead(Filename::extension(uri.pathView()));
  }

  MeshLoader::MeshLoader(GraphicsDevice * pDevice)
    : m_pGraphics(pDevice) {}

  Ref<Mesh> MeshLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    Ref<MeshData> pData = pContext->load<MeshData>(uri);
    if (pData == nullptr) {
      return nullptr;
    }

    Ref<Mesh> pMesh    = NewRef<Mesh>();
    auto      pCmdList = m_pGraphics->createCommandList();

    if (!pMesh->load(pCmdList.get(), *pData)) {
      return nullptr;
    }

    m_pGraphics->submit(std::move(pCmdList));

    return pMesh;
  }

  bool MeshLoader::handles(URI const & uri, AssetManager const * pManager) const {
    return pManager->canLoad<MeshData>(uri);
  }

  MeshMaterialLoader::MeshMaterialLoader(GraphicsDevice * pDevice)
    : m_pGraphics(pDevice) {}

  Ref<Material> MeshMaterialLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    StringView indexString = uri.fragment().substr(fragmentPrefix.length());
    int64_t         index       = Scan::readInt(&indexString);
    if (indexString.size() > 0 || index < 0) {
      return nullptr;
    }

    Ref<MeshData> pData = pContext->load<MeshData>(URI::File(uri.path()));
    if (pData == nullptr) {
      return nullptr;
    }

    if (index >= pData->materials.size()) {
      return nullptr;
    }

    auto pCmdList = m_pGraphics->createCommandList();

    Ref<Material> pMaterial = NewRef<Material>();
    pMaterial->load(
      pCmdList.get(),
      pData->materials[index],
      [pContext](MeshData::Material const & data, String const & prop) -> Ref<graphics::Texture> {
        return pContext->load<graphics::Texture>(URI::File(data.getTexture(prop)));
      }
    );
    pMaterial->upload(pCmdList.get());

    m_pGraphics->submit(std::move(pCmdList));

    return pMaterial;
  }

  bool MeshMaterialLoader::handles(URI const & uri, AssetManager const * pManager) const {
    StringView fragment = uri.fragment();
    return fragment.startsWith(fragmentPrefix) && pManager->canLoad<MeshData>(uri);
  }
} // namespace engine
