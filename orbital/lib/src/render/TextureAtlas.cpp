#include "render/TextureAtlas.h"

namespace bfc {
  TextureAtlas::TextureAtlas(Vec2i resolution, Vec2i gridDims)
    : m_resolution(resolution)
    , m_gridDims(gridDims)
    , m_fullRes(m_resolution * m_gridDims)
  {}

  bool TextureAtlas::add(graphics::CommandList * pCmdList, URI const & uri) {
    media::Surface surface;
    if (!loadSurface(uri, &surface)) {
      return false;
    }

    bool success = add(pCmdList, uri, surface);
    surface.free();
    return success;
  }

  bool TextureAtlas::add(graphics::CommandList * pCmdList, URI const & uri, media::Surface const & surface) {
    if (m_lookup.contains(uri)) {
      return false;
    }

    if (m_free.size() == 0) {
      allocateTexture(pCmdList);
    }

    Cell newCell = m_free.popBack();
    graphics::TextureRef &tex = m_textures[newCell.texture];
    media::Surface dst = surface;
    dst.size           = { m_resolution, 1 };

    if (dst.size != surface.size) {
      dst.pBuffer = nullptr;
      convertSurface(&dst, surface);
      graphics::loadTextureSub2D(pCmdList, &tex, dst, Vec2i(Vec2(m_fullRes) * newCell.topLeft));
      dst.free();
    } else {
      graphics::loadTextureSub2D(pCmdList, &tex, surface, Vec2i(Vec2(m_fullRes) * newCell.topLeft));
    }

    m_lookup.add(uri, newCell);
    return true;
  }

  bool TextureAtlas::get(URI const & uri, graphics::TextureRef *ppTexture, Vec2 * pTopLeft, Vec2 * pBotRight) const {
    Cell const * pCell = m_lookup.tryGet(uri);
    if (pCell == nullptr) {
      return false;
    }

    if (ppTexture != nullptr) {
      *ppTexture = m_textures[pCell->texture];
    }

    if (pTopLeft != nullptr) {
      *pTopLeft = pCell->topLeft;
    }

    if (pBotRight != nullptr) {
      *pBotRight = pCell->botRight;
    }

    return true;
  }

  Vector<URI> TextureAtlas::getTextures() const {
    return m_lookup.getKeys();
  }

  void TextureAtlas::allocateTexture(graphics::CommandList * pCmdList) {
    graphics::TextureRef newTexture;
    graphics::loadTexture2D<RGBAu8>(pCmdList, &newTexture, m_resolution * m_gridDims);

    for (int64_t y = m_gridDims.y - 1; y >= 0; --y) {
      for (int64_t x = m_gridDims.x - 1; x >= 0; --x) {
        Cell cell;
        cell.topLeft  = Vec2(x, y) / Vec2(m_gridDims);
        cell.botRight = Vec2(x + 1, y + 1) / Vec2(m_gridDims);
        cell.texture  = m_textures.size();
        m_free.pushBack(cell);
      }
    }

    m_textures.pushBack(newTexture);
  }
}
