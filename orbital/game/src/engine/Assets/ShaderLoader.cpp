#include "ShaderLoader.h"
#include "../Assets/AssetLoadContext.h"
#include "../Assets/AssetManager.h"

using namespace bfc;

namespace engine {
  ShaderLoader::ShaderLoader(GraphicsDevice * pGraphicsDevice)
    : m_pGraphicsDevice(pGraphicsDevice) {}

  Ref<Shader> ShaderLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    auto definition = pContext->getFileSystem()->deserialize<ShaderDefinition>(uri);
    if (!definition.has_value()) {
      return nullptr;
    }

    URI sourceBasePath = uri.withPath(uri.path().parent());
    Map<ShaderType, Filename> sources;
    for (auto & [type, shaderFile] : definition->sources) {
      URI resolvedUri = pContext->getFileSystem()->resolveUri(sourceBasePath.resolveRelativeReference(shaderFile));
      sources.add(type, resolvedUri.path());
    }

    Ref<Shader> shader = NewRef<Shader>();
    // TODO: Use URI to load shader source and customize reading files.
    if (!shader->loadFiles(m_pGraphicsDevice, sources)) {
      return nullptr;
    }

    return shader;
  }

  bool ShaderLoader::handles(URI const& uri, AssetManager const *) const {
    return Filename::extension(uri.pathView()).equals("shader", true);
  }
} // namespace engine
