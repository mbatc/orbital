#pragma once

#include "../media/Surface.h"
#include "../media/Image.h"
#include "../core/Map.h"
#include "../render/Texture.h"

namespace bfc {
  class GraphicsDevice;

  /// Simple texture atlas consisting of uniform grid cells.
  class BFC_API TextureAtlas {
  public:
    /// Construct a texture atlas.
    /// @param resolution The resolution of each cell in the atlas.
    /// @param gridDims   The dimensions of the grid for each texture.
    TextureAtlas(GraphicsDevice *pDevice, Vec2i resolution, Vec2i gridDims);
    /// Load a uri.
    bool add(URI const & uri);
    /// Add a texture to the atlas.
    /// The surface will be scaled and to the resolution of the atlas.
    /// @param uri The URI used to reference the texture.
    bool add(URI const & uri, media::Surface const & surface);
    /// Get the texture, and the uv coordinates for a texture in the atlas.
    bool get(URI const & uri, Texture * pTexture, Vec2 * pTopLeft = nullptr, Vec2 * pBotRight = nullptr) const;
    /// Get a list of all the URIs in the atlas.
    Vector<URI> getTextures() const;

  private:
    void allocateTexture();

    struct Cell {
      int64_t texture;
      Vec2    topLeft;
      Vec2    botRight;
    };

    Vec2i m_resolution;
    Vec2i m_gridDims;
    Vec2i m_fullRes;

    Map<URI, Cell>  m_lookup;
    Vector<Cell>    m_free;
    Vector<Texture> m_textures;

    GraphicsDevice * m_pDevice = nullptr;
  };
}
