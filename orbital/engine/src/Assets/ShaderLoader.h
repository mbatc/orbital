#pragma once

#include "core/Map.h"
#include "core/URI.h"
#include "core/Serialize.h"
#include "render/GraphicsDevice.h"

#include "AssetLoader.h"

namespace engine {
  class ShaderDefinition {
  public:
    bfc::Map<bfc::ShaderType, bfc::URI> sources;
  };

  class ShaderLoader : public AssetLoader<bfc::graphics::Program> {
  public:
    virtual bfc::Ref<bfc::graphics::Program> load(bfc::URI const & uri, AssetLoadContext * pContext) const override;
    virtual bool                  handles(bfc::URI const & uri) const override;
  };
} // namespace engine

namespace bfc {
  template<>
  struct Serializer<engine::ShaderDefinition> {
    template<typename Context>
    static SerializedObject write(engine::ShaderDefinition const & o, Context const &) {
      return SerializedObject::MakeMap({
         { "src", serialize(o.sources) },
      });
    }

    template<typename Context>
    static bool read(SerializedObject const & s, engine::ShaderDefinition & o, Context const &) {
      return s.get("src").read(o.sources);
    }
  };
}
