#include "RenderData.h"

namespace engine {
  RenderData::RenderData(bfc::GraphicsDevice * pGraphicsDevice) 
  : m_pGraphics(pGraphicsDevice) {
    m_pUpload = m_pGraphics->createCommandList();
  }

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
    m_pUpload = nullptr;
  }

  bfc::graphics::CommandList * RenderData::getUploadCommandList() {
    if (m_pUpload == nullptr) {
      m_pUpload = m_pGraphics->createCommandList();
    }
    return m_pUpload.get();
  }

  void RenderData::submitUploadList() {
    m_pGraphics->submit(std::move(m_pUpload));
  }

  bfc::GraphicsDevice * RenderData::getGraphicsDevice() const {
    return m_pGraphics;
  }
} // namespace engine
