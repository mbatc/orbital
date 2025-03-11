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
    DeferredRenderer(bfc::graphics::CommandList * pCmdList, AssetManager * pAssets);

    static constexpr int64_t ColourTargetBindPointBase  = 0;
    static constexpr int64_t FinalColourTargetBindPoint = ColourTargetBindPointBase + bfc::GBufferTarget_Count;
    static constexpr int64_t DepthTargetBindPoint       = FinalColourTargetBindPoint + 1;

    virtual void onResize(bfc::graphics::CommandList *pCmdList, bfc::Vec2i size) override;
    virtual void render(bfc::graphics::CommandList * pCmdList, bfc::Vector<RenderView> const & views) override;

    /// Get the GBuffer render target.
    bfc::graphics::RenderTargetRef getFinalTarget() const;

    /// Get the default texture for a texture slot.
    bfc::graphics::TextureRef const & getDefaultTexture(bfc::Material::TextureSlot slot) const;

    /// Get the default material data
    bfc::graphics::StructuredBuffer<bfc::renderer::PBRMaterial> const & getDefaultMaterial() const;

  protected:
    virtual void beginView(bfc::graphics::CommandList * pCmdList, RenderView const & view) override;
    virtual void endView(bfc::graphics::CommandList * pCmdList, RenderView const & view) override;

  private:
    bfc::PostProcessingStack  m_postProc;
    bfc::GBuffer              m_gbuffer;
    bfc::graphics::TextureRef m_finalColourTarget; ///< Final scene colour is rendered to this target.

    bfc::graphics::RenderTargetRef m_finalTarget = bfc::InvalidGraphicsResource;

    bfc::graphics::StructuredBuffer<bfc::renderer::PBRMaterial>  m_defaultMaterial;
    bfc::graphics::StructuredBuffer<bfc::renderer::ModelBuffer>  m_modelData;
    bfc::graphics::StructuredBuffer<bfc::renderer::CameraBuffer> m_cameraData;

    bfc::graphics::TextureRef m_defaultMaterialTextures[bfc::Material::TextureSlot_Count]; // Default textures used if missing
  };
} // namespace engine
