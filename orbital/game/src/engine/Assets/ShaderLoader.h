#pragma once

#include "core/Map.h"
#include "core/URI.h"
#include "core/Serialize.h"
#include "render/Shader.h"

#include "AssetLoader.h"

namespace engine {
  class ShaderDefinition {
  public:
    bfc::Map<bfc::ShaderType, bfc::URI> sources;
  };

  class ShaderLoader : public AssetLoader<bfc::Shader> {
  public:
    ShaderLoader(bfc::GraphicsDevice * pGraphicsDevice);

    virtual bfc::Ref<bfc::Shader> load(bfc::URI const & uri, AssetLoadContext * pContext) const override;
    virtual bool                  handles(bfc::URI const & uri, AssetManager const * pManager) const override;

  private:
    bfc::GraphicsDevice * m_pGraphicsDevice = nullptr;
  };
} // namespace engine

namespace bfc {
  template<>
  struct Serializer<engine::ShaderDefinition> {
    inline static SerializedObject write(engine::ShaderDefinition const & o) {
      return SerializedObject::MakeMap({
         { "src", serialize(o.sources) },
      });
    }

    inline static bool read(SerializedObject const & s, engine::ShaderDefinition & o) {
      return s.get("src").read(o.sources);
    }
  };
}
