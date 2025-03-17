#pragma once

#include "core/Map.h"
#include "core/Vector.h"
#include "geometry/Geometry.h"
#include "math/MathTypes.h"
#include "render/GraphicsDevice.h"
#include "render/HardwareBuffer.h"

#include <typeindex>

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
    T * addFeature(Args... args) {
      T * pRenderer = new T(args...);
      m_features.pushBack(pRenderer);
      m_added.pushBack(pRenderer);
      return pRenderer;
    }

    /// Resize the targets for the renderer.
    /// Ideally we could remove this, but for now this is an easy way to resize intermediate resources.
    virtual void onResize(bfc::graphics::CommandList * pCmdList, bfc::Vec2i size) {
      for (auto * pFeature : m_features) {
        pFeature->onResize(pCmdList, this, size);
      }
    }

    /// Render a collection of RenderView's
    virtual void render(bfc::graphics::CommandList * pCmdList, bfc::Vector<RenderView> const & views);

    /// Get the number of features in the renderer.
    int64_t numFeatures() const;

    /// Get a feature in the renderer.
    FeatureRenderer * getFeature(int64_t index) const;

    /// Get the renderers graphics device.
    bfc::GraphicsDevice * getGraphicsDevice() const;

  protected:
    virtual void beginView(bfc::graphics::CommandList * pCmdList, RenderView const & view);
    virtual void endView(bfc::graphics::CommandList * pCmdList, RenderView const & view);

  private:
    bfc::GraphicsDevice * m_pDevice = nullptr;

    bfc::Vector<FeatureRenderer *> m_features;

    // Features added to the renderer.
    // These will be initialised on the next render.
    bfc::Vector<FeatureRenderer *> m_added;
  };
} // namespace engine
