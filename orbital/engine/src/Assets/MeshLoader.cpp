#include "MeshLoader.h"
#include "AssetManager.h"
#include "Rendering/Rendering.h"

#include "mesh/Mesh.h"
#include "mesh/parsers/OBJParser.h"
#include "mesh/parsers/FBXParser.h"

#include "util/Scan.h"

using namespace bfc;

namespace engine {
  Ref<MeshData> MeshDataFileLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    BFC_UNUSED(pContext);

    Ref<Stream> pStream = fs::open(uri, FileMode_ReadBinary);
    if (pStream == nullptr) {
      return nullptr;
    }

    Ref<MeshData> pMeshData = NewRef<MeshData>();

    bool       success    = false;
    StringView ext        = Filename::extension(uri.pathView());
    Filename   parentPath = Filename::parent(fs::resolveUri(uri).path());

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
      return fs::find(URI::File(file), {uri.resolveRelativeReference("../")}).path();
    });

    pMeshData->findTextures();

    return pMeshData;
  }

  bool MeshDataFileLoader::handles(URI const & uri) const {
    return MeshData::canRead(Filename::extension(uri.pathView()));
  }

  Ref<Mesh> MeshLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    Ref<MeshData> pData = asset_manager::load<MeshData>(uri, nullptr, pContext);
    if (pData == nullptr) {
      return nullptr;
    }

    Ref<Mesh> pMesh    = NewRef<Mesh>();
    auto      pCmdList = rendering::createCommandList();
    pCmdList->setDebugName("MeshLoader::load");

    if (!pMesh->load(pCmdList.get(), *pData)) {
      return nullptr;
    }

    rendering::submit(std::move(pCmdList));

    return pMesh;
  }

  bool MeshLoader::handles(URI const & uri) const {
    return asset_manager::canLoad<MeshData>(uri);
  }

  Ref<Material> MeshMaterialLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    StringView indexString = uri.fragment().substr(fragmentPrefix.length());
    int64_t         index       = Scan::readInt(&indexString);
    if (indexString.size() > 0 || index < 0) {
      return nullptr;
    }

    Ref<MeshData> pData = asset_manager::load<MeshData>(URI::File(uri.path()), nullptr, pContext);
    if (pData == nullptr) {
      return nullptr;
    }

    if (index >= pData->materials.size()) {
      return nullptr;
    }

    auto pCmdList = rendering::createCommandList();
    pCmdList->setDebugName("MeshMaterialLoader::load");

    Ref<Material> pMaterial = NewRef<Material>();
    pMaterial->load(
      pCmdList.get(),
      pData->materials[index],
      [pContext](MeshData::Material const & data, String const & prop) -> Ref<graphics::Texture> {
        return asset_manager::load<graphics::Texture>(URI::File(data.getTexture(prop)));
      }
    );
    pMaterial->upload(pCmdList.get());

    rendering::submit(std::move(pCmdList));

    return pMaterial;
  }

  bool MeshMaterialLoader::handles(URI const & uri) const {
    StringView fragment = uri.fragment();
    return fragment.startsWith(fragmentPrefix) && asset_manager::canLoad<MeshData>(uri);
  }

  Ref<MeshData> MeshDataCache::read(bfc::Stream * pStream) const {
    auto mesh = bfc::read<MeshData>(pStream);
    if (mesh.has_value())
      return bfc::NewRef<MeshData>(std::move(mesh.value()));
    return nullptr;
  }

  bool MeshDataCache::store(bfc::Ref<MeshData> pAsset, bfc::Stream * pStream) const {
    return pStream->write(*pAsset.get());
  }
} // namespace engine
