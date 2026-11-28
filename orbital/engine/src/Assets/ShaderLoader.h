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
  struct Serialization<engine::ShaderDefinition> {
    template<typename Serializer>
    static SerializedObject write(Serializer & ar, engine::ShaderDefinition const & o) {
      return SerializedObject::MakeMap({
         { "src", ar(o.sources) },
      });
    }

    template<typename Serializer>
    static bool read(Serializer & ar, SerializedObject const & s, engine::ShaderDefinition & o) {
      return ar(s.get("src"), o.sources);
    }
  };
}
