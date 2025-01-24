#pragma once

#include "../core/Core.h"
#include "../core/Filename.h"
#include "../render/GraphicsDevice.h"

#include <mutex>

namespace bfc {
  class BFC_API Shader : public ManagedGraphicsResource {
  public:
    Shader() = default;
    Shader(GraphicsDevice* pDevice, Map<ShaderType, String> const& source);
    Shader(GraphicsDevice* pDevice, Map<ShaderType, Filename> const& files);

    bool load(GraphicsDevice* pDevice, Map<ShaderType, String> const& source);
    bool loadFiles(GraphicsDevice* pDevice, Map<ShaderType, Filename> const & files);

    bool setUniform(StringView const & name, Mat4 value);
    bool setUniform(StringView const & name, float value);
    bool setUniform(StringView const & name, Vec2 value);
    bool setUniform(StringView const & name, Vec3 value);
    bool setUniform(StringView const & name, Vec4 value);
    bool setUniform(StringView const & name, Vec2i value);
    bool setUniform(StringView const & name, Vec3i value);
    bool setUniform(StringView const & name, Vec4i value);
    bool setUniform(StringView const & name, int32_t value);

    bool setTextureBinding(StringView const& name, int64_t bindPoint);
    bool setBufferBinding(StringView const& name, int64_t bindPoint);

  private:
    bool setUniform(StringView const& name, void const* value);
    bool compileAndLink(GraphicsDevice* pDevice, Vector<GraphicsResource> const & shaders);
  };
}
