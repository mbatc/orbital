#pragma once

#include "Renderer.h"
#include "rendering/Renderables.h"

#include "mesh/Mesh.h"
#include "render/GBuffer.h"
#include "render/GraphicsDevice.h"
#include "render/PostProcessingStack.h"
#include "render/RendererCommon.h"

namespace bfc {
  class Mesh;
  class Material;
} // namespace bfc

namespace engine {
  class LightComponent;
  class Scene;
  class AssetManager;

  struct ShadowMapData {
    bfc::Mat4                     lightVP;
    bfc::geometry::Frustum<float> lightFrustum;

    int64_t          lightIndex = 0;
    bfc::CubeMapFace cubeFace   = bfc::CubeMapFace_None; // Cube map face for point lights
    int64_t          atlasIndex = -1;

    LightRenderable light;
    float           maxDistance = 0.0f; // max distance the light will influence geometry from

    bfc::geometry::Boxf casterBounds;
  };

  class DeferredRenderer : public Renderer {
  public:
    struct Stages {
      struct BasePassRequest {
        bfc::GBuffer * pGBuffer = nullptr;
      };

      struct Transparency {
        struct Depth {
          bfc::graphics::RenderTarget * pTarget = nullptr;
        };

        struct Transmittance {
          bfc::GBuffer * pOpaqueGBuffer = nullptr;
          bfc::GBuffer * pTargetBuffer = nullptr;
        };
      };

      struct ShadowDepth {
        ShadowMapData const * pShadowData = nullptr;
      };

      struct ShadowReceiverBounds {
        bfc::geometry::Frustum<float> cameraFrustum;
        bfc::geometry::Boxf *         pBounds = nullptr;
      };

      struct ShadowCasterBounds {
        bfc::geometry::Frustum<float> lightFrustum;
        bfc::geometry::Boxf * pBounds = nullptr;
        LightRenderable       light;
      };

      struct ShadowCasterOrthoBounds {
        bfc::geometry::Boxf   receiverBounds;
        bfc::geometry::Boxf   receiverBoundsLightSpace;
        bfc::geometry::Boxf * pBounds = nullptr;
        bfc::Vec3        right, up;
        LightRenderable  light;
      };

      struct ShadowMesh {};

      struct PostProcess {
        struct Pre {
          bfc::PostProcessInput          input;
          bfc::graphics::TextureRef pFinalColour;
        };

        struct Perform {
          bfc::PostProcessingStack * pPostProcessing = nullptr;
        };

        struct Post {
          bfc::graphics::RenderTargetRef pTarget;
        };
      };
    };

    struct Phase {
      inline static const bfc::StringView undefined = "undefined";
      struct Base {
        struct Mesh {
          inline static const bfc::StringView opaque      = "base.mesh.opaque";
          inline static const bfc::StringView transparent = "base.mesh.transparent";
        };
      };
      inline static const bfc::StringView lighting        = "lighting";
      inline static const bfc::StringView skybox          = "skybox";
      inline static const bfc::StringView postProcess     = "post-process";
    };

    DeferredRenderer(bfc::graphics::CommandList * pCmdList, AssetManager * pAssets);

    static constexpr int64_t ColourTargetBindPointBase  = 8;
    static constexpr int64_t FinalColourTargetBindPoint = ColourTargetBindPointBase + bfc::GBufferTarget_Count;
    static constexpr int64_t DepthTargetBindPoint       = FinalColourTargetBindPoint + 1;

    virtual void onResize(bfc::graphics::CommandList * pCmdList, bfc::Vec2i size) override;

    virtual void render(bfc::graphics::CommandList * pCmdList, bfc::Vector<RenderView> const & views) override;

    /// Get the GBuffer render target.
    // bfc::graphics::RenderTargetRef getFinalTarget() const;

    /// Get the default texture for a texture slot.
    bfc::graphics::TextureRef const & getDefaultTexture(bfc::Material::TextureSlot slot) const;

    /// Get the default material data
    bfc::graphics::StructuredBuffer<bfc::renderer::PBRMaterial> const & getDefaultMaterial() const;

  protected:
    virtual void beginView(bfc::graphics::CommandList * pCmdList, RenderView const & view) override;
    virtual void endView(bfc::graphics::CommandList * pCmdList, RenderView const & view) override;

  private:
    bfc::Ref<bfc::GBuffer> m_pGbuffer;
    bfc::Ref<bfc::GBuffer> m_pTransparencyGBuffer;
    bfc::graphics::TextureRef m_finalColourTarget; ///< Final scene colour is rendered to this target.

    bfc::graphics::RenderTargetRef m_finalTarget = bfc::InvalidGraphicsResource;

    bfc::graphics::StructuredBuffer<bfc::renderer::PBRMaterial>  m_defaultMaterial;
    bfc::graphics::StructuredBuffer<bfc::renderer::ModelBuffer>  m_modelData;
    bfc::graphics::StructuredBuffer<bfc::renderer::CameraBuffer> m_cameraData;

    bfc::graphics::TextureRef m_defaultMaterialTextures[bfc::Material::TextureSlot_Count]; // Default textures used if missing
  };
} // namespace engine
