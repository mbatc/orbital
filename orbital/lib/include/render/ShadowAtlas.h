#pragma once

#include "../math/MathTypes.h"
#include "../core/Pool.h"
#include "GraphicsDevice.h"

namespace bfc {
  class GraphicsDevice;

  /// A shadow texture 
  class BFC_API ShadowAtlas {
  public:
    struct Slot {
      int8_t layer = -1; ///< Index in LOD level
      int8_t level = -1; ///< LOD level in atlas
    };

    ShadowAtlas(graphics::CommandList * pDevice, int64_t maxRes = 2048, int64_t memoryLimit = 128 * 1024 * 1024 /*128 Mb*/);

    int64_t allocate(float priority);

    void release(int64_t slot);
    void pack();

    Slot   getSlot(int64_t allocation) const;
    Vec2i  resolution() const;
    Vec2i  resolution(int64_t allocation) const;

    inline operator graphics::TextureRef() const {
      return m_texture;
    }

  private:
    Pool<Pair<float, Slot>> m_allocations;

    Vec2i   m_resolution = {0,0};
    int64_t m_numLayers = 0;
    int64_t m_numMips = 0;

    graphics::TextureRef m_texture; // 3D texture
    GraphicsDevice * m_pDevice = nullptr;
  };
}
