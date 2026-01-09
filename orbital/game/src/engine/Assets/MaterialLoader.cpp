#include "MaterialLoader.h"
#include "AssetManager.h"
#include "AssetLoadContext.h"

#include "mesh/Mesh.h"
#include "util/Scan.h"

using namespace bfc;

namespace engine {
  Ref<MeshData::Material> MaterialFileLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    auto material = pContext->getFileSystem()->deserialize<MeshData::Material>(uri);

    if (!material.has_value()) {
      return nullptr;
    }

    return bfc::NewRef<MeshData::Material>(std::move(material.value()));
  }

  bool MaterialFileLoader::handles(URI const & uri, AssetManager const * pManager) const {
    BFC_UNUSED(pManager);

    return Filename::extension(uri.pathView()) == "material";
  }

  MaterialLoader::MaterialLoader(GraphicsDevice * pDevice)
    : m_pGraphics(pDevice) {}

  Ref<Material> MaterialLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    Ref<MeshData::Material> pData = pContext->load<MeshData::Material>(uri);
    if (pData == nullptr) {
      return nullptr;
    }

    Ref<Material> pMaterial = NewRef<Material>();
    auto          pCmdList  = m_pGraphics->createCommandList();

    pMaterial->load(pCmdList.get(), *pData);

    m_pGraphics->submit(std::move(pCmdList));

    return pMaterial;
  }

  bool MaterialLoader::handles(URI const & uri, AssetManager const * pManager) const {
    return pManager->canLoad<MeshData::Material>(uri);
  }
} // namespace engine
