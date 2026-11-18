#include "ShaderLoader.h"
#include "Assets/AssetManager.h"
#include "Rendering/Rendering.h"

using namespace bfc;

namespace engine {
  Ref<graphics::Program> ShaderLoader::load(URI const & uri, AssetLoadContext * pContext) const {
    auto definition = fs::deserialize<ShaderDefinition>(uri);
    if (!definition.has_value()) {
      return nullptr;
    }

    URI sourceBasePath = uri.withPath(uri.path().parent());
    Map<ShaderType, URI> sources;
    for (auto & [type, shaderFile] : definition->sources) {
      URI resolvedUri = fs::resolveUri(sourceBasePath.resolveRelativeReference(shaderFile));
      sources.add(type, resolvedUri);
    }

    graphics::ProgramRef shader = rendering::createProgram();
    shader->setFiles(sources);

    // TODO: Use URI to load shader source and customize reading files.
    if (!rendering::compile(shader).get()) {
      return nullptr;
    }

    return shader;
  }

  bool ShaderLoader::handles(URI const& uri) const {
    return Filename::extension(uri.pathView()).equals("shader", true);
  }
} // namespace engine
