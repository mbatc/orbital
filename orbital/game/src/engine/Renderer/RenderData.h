#pragma once

#include "RenderableStorage.h"
#include "math/MathTypes.h"
#include "render/GraphicsDevice.h"
#include "render/HardwareBuffer.h"

#include <typeindex>

namespace bfc {
  class ShaderPool;
}

namespace engine {
  /// This class stores renderable data
  class RenderData {
  public:
    ~RenderData();

    /// Get storage for a specific renderable type.
    template<typename T>
    RenderableStorage<T> & renderables() {
      RenderableStorageBase *& pStorage = m_renderables.getOrAdd(TypeID<T>());
      if (pStorage == nullptr) {
        pStorage = new RenderableStorage<T>;
      }

      return (*(RenderableStorage<T> *)pStorage);
    }

    /// Get const storage for a specific renderable type.
    template<typename T>
    RenderableStorage<T> const & renderables() const {
      RenderableStorageBase * pStorage = m_renderables.getOr(TypeID<T>(), nullptr);
      if (pStorage == nullptr) {
        static RenderableStorage<T> tmp;
        return tmp;
      }

      return (*(RenderableStorage<T> *)pStorage);
    }

    /// Clear all the renderable data.
    void clear();

    /// Add a renderable to the render data.
    template<typename T>
    void submit(T const & renderable) {
      renderables<T>().pushBack(renderable);
    }

    bfc::geometry::Spheref calcBoundingSphere() const {
      bfc::geometry::Spheref s;
      for (auto & [type, pStorage] : m_renderables) {
        s.growToContain(pStorage->calcBoundingSphere(0, pStorage->size()));
      }
      return s;
    }

    template<typename... Types>
    bfc::geometry::Spheref calcBoundingSphere() const {
      bfc::geometry::Spheref s;
      (s.growToContain(renderables<Types>().calcBoundingSphere()), ...);
      return s;
    }

    bfc::geometry::Boxf calcBoundingBox() const {
      bfc::geometry::Boxf b;
      for (auto & [type, pStorage] : m_renderables) {
        b.growToContain(pStorage->calcBoundingBox(0, pStorage->size()));
      }
      return b;
    }

    template<typename... Types>
    bfc::geometry::Boxf calcBoundingBox() const {
      bfc::geometry::Boxf b;
      (b.growToContain(renderables<Types>().calcBoundingBox()), ...);
      return b;
    }

  private:
    bfc::Map<bfc::type_index, RenderableStorageBase *> m_renderables; ///< Lookup renderable list by type.
  };
} // namespace engine
