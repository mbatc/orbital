#pragma once

#include "GraphicsDevice.h"

namespace bfc {
  class BFC_API HardwareBuffer : public ManagedGraphicsResource {
  public:
    HardwareBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown);

    /// Upload data to a hardware buffer.
    void upload(GraphicsDevice * pDevice, int64_t size, void * pData = nullptr);
    int64_t getSize() const;
    void *  map(MapAccess access = MapAccess_ReadWrite) const;
    void *  map(int64_t offset, int64_t size, MapAccess access = MapAccess_ReadWrite) const;
    void    unmap() const;
    int64_t download(void * pDst, int64_t offset = 0, int64_t size = 0) const;

    BufferUsageHint getUsageHint() const;

  private:
    BufferUsageHint m_hint = BufferUsageHint_Unknown;
  };

  template<typename Type>
  class StructuredHardwareBuffer : public ManagedGraphicsResource {
  public:
    StructuredHardwareBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown)
      : m_hint(usageHint) {}

    Type data;

    void upload(GraphicsDevice *pDevice) {
      graphics::BufferManager * pBuffers = pDevice->getBufferManager();
      GraphicsResource          resource  = getResource();
      if (resource == InvalidGraphicsResource) {
        resource = pBuffers->createBuffer(m_hint);
        pBuffers->upload(resource, sizeof(Type), &data);
      } else {
        void * pMapped = pBuffers->map(resource, MapAccess_Write);
        memcpy(pMapped, &data, sizeof(Type));
        pBuffers->unmap(resource);
      }

      set(pDevice, resource);
    }

    BufferUsageHint getUsageHint() const {
      return m_hint;
    }

  private:
    BufferUsageHint m_hint = BufferUsageHint_Unknown;
  };

  template<typename Type>
  class StructuredHardwareArrayBuffer : public ManagedGraphicsResource {
  public:
    StructuredHardwareArrayBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown)
      : m_hint(usageHint)
    {}

    Vector<Type> data;

    void upload(GraphicsDevice * pDevice) {
      graphics::BufferManager * pBuffers = pDevice->getBufferManager();
      GraphicsResource          resource = getResource();

      if (resource == InvalidGraphicsResource) {
        resource = pBuffers->createBuffer(m_hint);
      }

      pBuffers->upload(resource, sizeof(Type) * data.size(), data.data());
      set(pDevice, resource);
    }

    BufferUsageHint getUsageHint() const {
      return m_hint;
    }

  private:
    BufferUsageHint m_hint = BufferUsageHint_Unknown;
  };
}
