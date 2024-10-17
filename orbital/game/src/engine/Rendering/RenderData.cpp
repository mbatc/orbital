#include "RenderData.h"

namespace engine {
  /// This class stores renderable data
  RenderData::~RenderData() {
    clear();
    for (auto & [type, pStorage] : m_renderables) {
      if (pStorage != nullptr) {
        delete pStorage;
      }
    }
    m_renderables.clear();
  }

  void RenderData::clear() {
    for (auto & [type, pStorage] : m_renderables) {
      pStorage->clear();
    }
  }
} // namespace engine
