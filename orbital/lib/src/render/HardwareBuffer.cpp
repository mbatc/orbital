#include "render/HardwareBuffer.h"

namespace bfc {
  HardwareBuffer::HardwareBuffer(BufferUsageHint usageHint)
    : m_hint(usageHint)
  {}

  void HardwareBuffer::upload(GraphicsDevice * pDevice, int64_t size, void * pData) {
    graphics::BufferManager * pBuffers = pDevice->getBufferManager();
    GraphicsResource          resource = getResource();

    if (resource == InvalidGraphicsResource) {
      resource = pBuffers->createBuffer(m_hint);
    }

    pBuffers->upload(resource, size, pData);
    set(pDevice, resource);
  }

  int64_t HardwareBuffer::getSize() const {
    return hasResource() ? getDevice()->getBufferManager()->getSize(getResource()) : 0;
  }

  void *  HardwareBuffer::map(MapAccess access) const {
    return hasResource() ? getDevice()->getBufferManager()->map(getResource(), access) : nullptr;
  }

  void * HardwareBuffer::map(int64_t offset, int64_t size, MapAccess access) const {
    return hasResource() ? getDevice()->getBufferManager()->map(getResource(), offset, size, access) : nullptr;
  }

  void HardwareBuffer::unmap() const {
    if (hasResource()) {
      getDevice()->getBufferManager()->unmap(getResource());
    }
  }

  int64_t HardwareBuffer::download(void * pDst, int64_t offset, int64_t size) const {
    return hasResource() ? getDevice()->getBufferManager()->download(getResource(), pDst, offset, size) : 0;
  }
  BufferUsageHint HardwareBuffer::getUsageHint() const {
    return m_hint;
  }
}
