#include "render/ShadowAtlas.h"

namespace bfc {
  ShadowAtlas::ShadowAtlas(GraphicsDevice * pDevice, int64_t maxRes, int64_t memoryLimit) {
    m_numLayers = math::max(1ll, memoryLimit / (4 * maxRes * maxRes));
    m_texture.load2DArray(pDevice, {maxRes, maxRes, m_numLayers}, DepthStencilFormat_D32);
    m_texture.generateMipmaps();
    m_resolution = {maxRes, maxRes};
    m_numMips    = 1 + (int64_t)glm::floor(glm::log2(maxRes));
  }

  int64_t ShadowAtlas::allocate(float priority) {
    int64_t index              = m_allocations.emplace();
    m_allocations[index].first = priority;
    return index;
  }

  void ShadowAtlas::release(int64_t slot) {
    m_allocations.erase(slot);
  }

  void ShadowAtlas::pack() {
    Vector<Pair<int64_t, float>> mapping;
    for (int64_t i = 0; i < m_allocations.capacity(); ++i) {
      if (m_allocations.isUsed(i)) {
        m_allocations[i].second = Slot();
        mapping.pushBack(Pair<int64_t, float>(i, m_allocations[i].first));
      }
    }

    std::sort(mapping.begin(), mapping.end(), [](Pair<int64_t, float> const & a, Pair<int64_t, float> const & b) { return a.second < b.second; });

    for (int64_t i = 0; i < m_numMips; ++i) {
      for (int64_t j = 0; j < m_numLayers; ++j) {
        if (mapping.empty()) {
          break;
        }
        int64_t allocation               = mapping.popBack().first;
        m_allocations[allocation].second = {(int8_t)j, (int8_t)i};
      }

      if (mapping.empty()) {
        break;
      }
    }
  }

  ShadowAtlas::Slot ShadowAtlas::getSlot(int64_t allocation) const {
    return m_allocations[allocation].second;
  }

  Vec2i ShadowAtlas::resolution() const {
    return m_resolution;
  }

  Vec2i ShadowAtlas::resolution(int64_t allocation) const {
    return m_resolution / (1 << m_allocations[allocation].second.level);
  }
} // namespace bfc
