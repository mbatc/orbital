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
    bool reload();

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

  class BFC_API ShaderPool {
  public:
    ShaderPool(GraphicsDevice* pDevice);

    void setPath(Vector<Filename> const& directories);
    Vector<Filename> const& getPath() const;

    bool add(StringView name, Map<ShaderType, String> const& sources);
    bool addFiles(StringView name, Map<ShaderType, Filename> const& files);
    bool isLoaded(StringView name);
    bool isRegistered(StringView name);
    /// Reload all shaders in the pool.
    void reload();

    Shader load(StringView name);

    void tryUnload();

  private:
    struct ShaderDef {
      Map<ShaderType, String> sources;
      Map<ShaderType, Filename> files;
      bool loadFiles;
    };

    Map<String, Shader> m_loaded;
    Map<String, ShaderDef> m_shaders;

    Vector<Filename> m_searchDirectories;

    GraphicsDevice* m_pDevice = nullptr;

    std::mutex m_lock;
  };
}
