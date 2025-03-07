#pragma once

#include "GraphicsDevice.h"

namespace bfc {
  namespace graphics {
    template<typename Type>
    class StructuredBuffer {
    public:
      StructuredBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown)
        : m_hint(usageHint) {}

      Type data;

      void upload(graphics::CommandList * pCmd) {
        // m_pBuffer = 
        pCmd->upload();
        graphics::BufferManager * pBuffers = pDevice->getBufferManager();
        GraphicsResource          resource = getResource();
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
      BufferUsageHint     m_hint = BufferUsageHint_Unknown;
      graphics::BufferRef m_pBuffer;
    };

    template<typename Type>
    class StructuredArrayBuffer : public ManagedGraphicsResource {
    public:
      StructuredArrayBuffer(BufferUsageHint usageHint = BufferUsageHint_Unknown)
        : m_hint(usageHint) {}

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
}
