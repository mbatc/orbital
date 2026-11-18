#include "MaterialLoader.h"
#include "AssetManager.h"
#include "../Rendering/Rendering.h"

#include "mesh/Mesh.h"
#include "util/Scan.h"

using namespace bfc;

namespace engine {
  Ref<MeshData::Material> MaterialFileLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    auto material = fs::deserialize<MeshData::Material>(uri);

    if (!material.has_value()) {
      return nullptr;
    }

    return bfc::NewRef<MeshData::Material>(std::move(material.value()));
  }

  bool MaterialFileLoader::handles(URI const & uri) const {
    return Filename::extension(uri.pathView()) == "material";
  }

  Ref<Material> MaterialLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    Ref<MeshData::Material> pData = asset_manager::load<MeshData::Material>(uri, nullptr, pContext);
    if (pData == nullptr) {
      return nullptr;
    }

    Ref<Material> pMaterial = NewRef<Material>();
    auto          pCmdList  = rendering::createCommandList();
    pCmdList->setDebugName("MaterialLoader::load");

    pMaterial->load(pCmdList.get(), *pData);

    rendering::submit(std::move(pCmdList));

    return pMaterial;
  }

  bool MaterialLoader::handles(URI const & uri) const {
    return asset_manager::canLoad<MeshData::Material>(uri);
  }
} // namespace engine
