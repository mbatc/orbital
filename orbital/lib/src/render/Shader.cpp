#include "render/shader.h"
#include "core/File.h"

namespace bfc {
  Shader::Shader(GraphicsDevice * pDevice, Map<ShaderType, String> const & source) {
    load(pDevice, source);
  }

  Shader::Shader(GraphicsDevice * pDevice, Map<ShaderType, Filename> const & files) {
    loadFiles(pDevice, files);
  }

  bool Shader::load(GraphicsDevice * pDevice, Map<ShaderType, String> const & sources) {
    graphics::ShaderManager * pShaders = pDevice->getShaderManager();

    Vector<GraphicsResource> shaderIDs;
    for (auto & [shader, src] : sources) {
      GraphicsResource id = pShaders->createShader(shader);
      pShaders->setSource(id, src);
      shaderIDs.pushBack(id);
    }
    
    bool result = compileAndLink(pDevice, shaderIDs);

    for (GraphicsResource &shader : shaderIDs)
      pShaders->releaseShader(&shader);

    return result;
  }

  bool Shader::loadFiles(GraphicsDevice * pDevice, Map<ShaderType, Filename> const & files) {
    graphics::ShaderManager * pShaders = pDevice->getShaderManager();

    Vector<GraphicsResource> shaderIDs;
    for (auto & [shader, src] : files) {
      GraphicsResource id = pShaders->createShader(shader);
      pShaders->setFile(id, src.path());
      shaderIDs.pushBack(id);
    }

    bool result = compileAndLink(pDevice, shaderIDs);

    for (GraphicsResource &shader : shaderIDs)
      pShaders->releaseShader(&shader);

    return result;
  }

  bool Shader::setUniform(StringView const & name, Mat4 value) {
    return setUniform(name, &value);
  }

  bool Shader::setUniform(StringView const & name, float value) {
    return setUniform(name, &value);
  }

  bool Shader::setUniform(StringView const & name, Vec2 value) {
    return setUniform(name, &value);
  }

  bool Shader::setUniform(StringView const & name, Vec3 value) {
    return setUniform(name, &value);
  }

  bool Shader::setUniform(StringView const & name, Vec4 value) {
    return setUniform(name, &value);
  }

  bool Shader::setUniform(StringView const & name, Vec2i value) {
    return setUniform(name, &value);
  }

  bool Shader::setUniform(StringView const & name, Vec3i value) {
    return setUniform(name, &value);
  }

  bool Shader::setUniform(StringView const & name, Vec4i value) {
    return setUniform(name, &value);
  }

  bool Shader::setUniform(StringView const & name, int32_t value) {
    return setUniform(name, &value);
  }

  bool Shader::setUniform(StringView const & name, void const * value) {
    auto *           pShaderManager = getDevice()->getShaderManager();
    GraphicsResource programID      = getResource();

    int64_t uniformCount = pShaderManager->getUniformCount(programID);
    for (int64_t i = 0; i < uniformCount; ++i) {
      ProgramUniformDesc desc;
      pShaderManager->getUniformDesc(programID, i, &desc);
      if (desc.name == name) {
        pShaderManager->setUniform(programID, i, value);
        return true;
      }
    }

    return false;
  }

  bool Shader::setTextureBinding(StringView const & name, int64_t bindPoint) {
    auto *           pShaderManager = getDevice()->getShaderManager();
    GraphicsResource programID      = getResource();
    int64_t          textureCount   = pShaderManager->getTextureCount(programID);
    for (int64_t i = 0; i < textureCount; ++i) {
      ProgramTextureDesc desc;
      pShaderManager->getTextureDesc(programID, i, &desc);
      if (desc.name == name) {
        pShaderManager->setTextureBinding(programID, i, bindPoint);
        return true;
      }
    }

    return false;
  }

  bool Shader::setBufferBinding(StringView const & name, int64_t bindPoint) {
    auto *           pShaderManager = getDevice()->getShaderManager();
    GraphicsResource programID      = getResource();
    int64_t          bufferCount    = pShaderManager->getBufferCount(programID);
    for (int64_t i = 0; i < bufferCount; ++i) {
      ProgramBufferDesc desc;
      pShaderManager->getBufferDesc(programID, i, &desc);
      if (desc.name == name) {
        pShaderManager->setBufferBinding(programID, i, bindPoint);
        return true;
      }
    }

    return false;
  }

  bool Shader::compileAndLink(GraphicsDevice * pDevice, Vector<GraphicsResource> const & shaderIDs) {
    graphics::ShaderManager * pManager = pDevice->getShaderManager();
    bool                      compiled = true;

    String error;
    for (GraphicsResource id : shaderIDs) {
      if (!pManager->compile(id, &error)) {
        compiled = false;
        printf("Failed to compile shader\nError: %s\n", error.c_str());
      }
    }

    bool             linked    = false;
    GraphicsResource programID = InvalidGraphicsResource;
    if (compiled) {
      programID = pManager->createProgram();

      for (GraphicsResource id : shaderIDs) {
        pManager->addShader(programID, id);
      }

      linked = pManager->linkProgram(programID, &error);

      if (!linked) {
        printf("Failed to link shader program\nError: %s\n", error.c_str());
      }
    }

    if (!linked) {
      if (programID != InvalidGraphicsResource) {
        pManager->releaseProgram(&programID);
      }

      return false;
    }

    set(pDevice, programID);

    pManager->releaseProgram(&programID);

    return true;
  }
} // namespace bfc
