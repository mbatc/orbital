#pragma once

#include "core/Map.h"
#include "core/Vector.h"
#include "geometry/Geometry.h"
#include "math/MathTypes.h"
#include "render/GraphicsDevice.h"
#include "core/typeindex.h"

namespace bfc {
  class ShaderPool;
}

namespace engine {
  class RenderData;

  class RenderView {
  public:
    bfc::Mat4d viewMatrix;       ///< Camera view matrix
    bfc::Mat4d projectionMatrix; ///< Camera projection matrix

    bfc::graphics::RenderTargetRef renderTarget; ///< Where to render to
    bfc::Vec4d                     viewport;     ///< Rect within renderTarget

    RenderData * pRenderData = nullptr; ///< Data to be rendered

    void updateCachedProperties();

    inline bfc::Vec3d const & getCameraPosition() const {
      return m_camera.position;
    }

    inline bfc::Vec3d const & getCameraForward() const {
      return m_camera.fwd;
    }

    inline bfc::Vec3d const & getCameraUp() const {
      return m_camera.up;
    }

    inline bfc::Vec3d const & getCameraRight() const {
      return m_camera.right;
    }

    inline bfc::geometry::Frustum<double> const & getCameraFrustum() const {
      return m_camera.frustum;
    }

    inline bfc::Mat4d const & getViewProjectionMatrix() const {
      return m_camera.viewProjectionMatrix;
    }

    inline bfc::Mat4d const & getInverseViewProjectionMatrix() const {
      return m_camera.inverseViewProjectionMatrix;
    }

    inline bfc::Mat4d const & getInverseViewMatrix() const {
      return m_camera.inverseViewMatrix;
    }

    inline bfc::Mat4d const & getInverseProjectionMatrix() const {
      return m_camera.inverseProjectionMatrix;
    }

  private:
    struct CameraCache {
      bfc::Vec3d position;
      bfc::Vec3d fwd;
      bfc::Vec3d up;
      bfc::Vec3d right;

      bfc::geometry::Frustum<double> frustum;

      bfc::Mat4d viewProjectionMatrix;
      bfc::Mat4d inverseViewProjectionMatrix;
      bfc::Mat4d inverseViewMatrix;
      bfc::Mat4d inverseProjectionMatrix;

      void set(bfc::Mat4d view, bfc::Mat4d projection);
    } m_camera;
  };

  class Renderer;

  /// Interface for a feature renderer.
  class FeatureRenderer {
  public:
    virtual ~FeatureRenderer() = default;

    /// Called when the feature is added to a renderer.
    /// This entry point can be used to add any data to the renderer that might be
    /// required by this feature (e.g. adding shaders to the renderers shader pool).
    virtual void onAdded(bfc::graphics::CommandList * pCmdList, Renderer * pRenderer);
    virtual void onResize(bfc::graphics::CommandList * pCmdList, Renderer * pRenderer, bfc::Vec2i const & size);

    virtual void beginFrame(bfc::graphics::CommandList * pCmdList, Renderer * pRenderer, bfc::Vector<RenderView> const & views);
    virtual void endFrame(bfc::graphics::CommandList * pCmdList, Renderer * pRenderer, bfc::Vector<RenderView> const & views);
    virtual void beginView(bfc::graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view);
    virtual void renderView(bfc::graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view);
    virtual void endView(bfc::graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view);
  };

  /// Core renderer implementation.
  /// A renderer is just a collection of FeatureRenderer's.
  /// The renderer calls the entry-points defined in FeatureRenderer.
  class Renderer {
  public:
    /// Construct a new renderer.
    /// @param pDevice  The graphics device the renderer should use.
    /// @param pShaders The shader pool the renderer should use.
    Renderer(bfc::GraphicsDevice * pDevice);

    /// Destroy the renderer.
    /// Destructs all features in the order they were added.
    virtual ~Renderer();

    /// Add a feature to the renderer.
    template<typename T, typename... Args>
    T * addFeature(bfc::StringView const & phase, Args... args) {
      T * pRenderer = new T(args...);
      if (m_phases.tryAdd(phase, {})) {
        m_phaseOrder.pushBack(phase);
      }
      m_phases[phase].pushBack(pRenderer);
      m_added.pushBack(pRenderer);
      return pRenderer;
    }

    /// Resize the targets for the renderer.
    /// Ideally we could remove this, but for now this is an easy way to resize intermediate resources.
    virtual void onResize(bfc::graphics::CommandList * pCmdList, bfc::Vec2i size) {
      for (auto & [phase, features] : m_phases) {
        for (auto & pFeature : features) {
          pFeature->onResize(pCmdList, this, size);
        }
      }
    }

    /// Render a collection of RenderView's
    virtual void render(bfc::graphics::CommandList * pCmdList, bfc::Vector<RenderView> const & views);

    /// Get the number of phases in the renderer
    int64_t numPhases() const;

    bfc::StringView getPhase(int64_t index) const;

    /// Get the number of features in the renderer.
    int64_t numFeatures(bfc::StringView const & phase) const;

    /// Get a feature in the renderer.
    FeatureRenderer * getFeature(bfc::StringView const & phase, int64_t index) const;

    /// Get the order that the phases are rendered in.
    bfc::Span<bfc::String> getPhaseOrder() const;

    /// Set the phase order. Any unspecified phases will not be rendered.
    void setPhaseOrder(bfc::Vector<bfc::String> const & phases);

    template<typename T>
    void setResource(bfc::StringView const & name, bfc::Ref<T> const & pResource) {
      auto &registered = m_resources.getOrAdd(name);
      registered.addOrSet(bfc::TypeID<T>(), pResource);
    }
    
    template<typename T>
    void setResource(bfc::StringView const & name, T * pResource) {
      return setResource(name, bfc::Ref<T>(pResource, [](T *) {}));
    }

    template<typename T>
    bfc::Ref<T> getResource(bfc::StringView const& name) const {
      auto * pRegistered = m_resources.tryGet(name);

      if (pRegistered == nullptr) {
        return nullptr;
      }

      return std::static_pointer_cast<T>(pRegistered->getOr(bfc::TypeID<T>(), nullptr));
    }

    /// Get the available resource names
    bfc::Span<bfc::StringView> listResources() const;

    /// Get the renderers graphics device.
    bfc::GraphicsDevice * getGraphicsDevice() const;

  protected:
    virtual void beginView(bfc::graphics::CommandList * pCmdList, RenderView const & view);
    virtual void endView(bfc::graphics::CommandList * pCmdList, RenderView const & view);

  private:
    bfc::GraphicsDevice * m_pDevice = nullptr;

    bfc::Vector<bfc::String>                              m_phaseOrder;
    bfc::Map<bfc::String, bfc::Vector<FeatureRenderer *>> m_phases;

    bfc::Map<bfc::String, bfc::Map<bfc::type_index, bfc::Ref<void>>> m_resources;

    // Features added to the renderer.
    // These will be initialised on the next render.
    bfc::Vector<FeatureRenderer *> m_added;
  };
} // namespace engine
