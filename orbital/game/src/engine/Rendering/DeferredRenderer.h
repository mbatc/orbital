#pragma once

#include "Renderer.h"

#include "mesh/Mesh.h"
#include "render/GraphicsDevice.h"
#include "render/HardwareBuffer.h"
#include "render/RendererCommon.h"
#include "render/Shader.h"
#include "render/GBuffer.h"
#include "render/PostProcessingStack.h"

namespace bfc {
  class Mesh;
  class Material;
} // namespace bfc

namespace engine {
  class LightComponent;
  class Scene;
  class AssetManager;

  class DeferredRenderer : public Renderer {
  public:
    DeferredRenderer(bfc::GraphicsDevice * pDevice, AssetManager * pAssets);

    static constexpr int64_t ColourTargetBindPointBase  = 0;
    static constexpr int64_t FinalColourTargetBindPoint = ColourTargetBindPointBase + bfc::GBufferTarget_Count;
    static constexpr int64_t DepthTargetBindPoint       = FinalColourTargetBindPoint + 1;

    virtual void onResize(bfc::Vec2i size) override;
    virtual void render(bfc::Vector<RenderView> const & views) override;

    /// Get the GBuffer render target.
    bfc::GraphicsResource getFinalTarget() const;

    /// Get the default texture for a texture slot.
    bfc::Texture const & getDefaultTexture(bfc::Material::TextureSlot slot) const;

    /// Get the default material data
    bfc::StructuredHardwareBuffer<bfc::renderer::PBRMaterial> const & getDefaultMaterial() const;

  protected:
    virtual void beginView(RenderView const & view) override;
    virtual void endView(RenderView const & view) override;

  private:
    bfc::PostProcessingStack m_postProc;
    bfc::GBuffer             m_gbuffer;
    bfc::Texture             m_finalColourTarget; ///< Final scene colour is rendered to this target.

    bfc::GraphicsResource m_finalTarget = bfc::InvalidGraphicsResource;

    bfc::StructuredHardwareBuffer<bfc::renderer::PBRMaterial>  m_defaultMaterial;
    bfc::StructuredHardwareBuffer<bfc::renderer::ModelBuffer>  m_modelData;
    bfc::StructuredHardwareBuffer<bfc::renderer::CameraBuffer> m_cameraData;

    bfc::Texture m_defaultMaterialTextures[bfc::Material::TextureSlot_Count]; // Default textures used if missing

    bfc::GraphicsDevice * m_pDevice  = nullptr;
    bfc::ShaderPool *     m_pShaders = nullptr;
  };
} // namespace engine
