#pragma once

#include "Viewport.h"

namespace bfc {
  class GraphicsDevice;
}

namespace engine {
  class Level;
  class DeferredRenderer;
  class AssetManager;
  class EditorCamera {
  public:
    EditorCamera(bfc::Mouse * pMouse, bfc::Keyboard * pKeyboard);

    void update(bfc::Timestamp dt);

    void setFOV(double fov);

    void setNearPlane(float nearPlane);

    void setFarPlane(float farPlane);

    bfc::Mat4d transformMat() const;

    bfc::Mat4d viewMat() const;

    bfc::Mat4d projectionMat(double aspect = 1) const;

    bfc::Mat4d viewProjectionMat(double aspect = 1) const;

    float speedMultiplier = 1.0f;

  private:
    double m_fov       = glm::radians(60.0);
    float  m_nearPlane = 0.01f;
    float  m_farPlane  = 1000.0f;

    bfc::Vec3d m_position = bfc::Vec3d(0);
    bfc::Vec3d m_ypr      = bfc::Vec3d(0);

    bfc::Mouse *    m_pMouse    = nullptr;
    bfc::Keyboard * m_pKeyboard = nullptr;

    bool m_hasMouse = false;
  };

  class LevelEditorViewport : public Viewport {
  public:
    LevelEditorViewport(bfc::GraphicsDevice * pGraphics, AssetManager * pAssets);

    virtual bfc::Vector<RenderView> collectViews(bfc::GraphicsResource renderTarget) const override;

    EditorCamera camera;
  };
} // namespace engine
