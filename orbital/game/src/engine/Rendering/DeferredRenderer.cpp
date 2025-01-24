#include "DeferredRenderer.h"
#include "RenderData.h"
#include "Renderables.h"

#include "geometry/Geometry.h"
#include "mesh/Mesh.h"
#include "render/ShadowAtlas.h"

#include "../Assets/AssetManager.h"

using namespace bfc;

namespace engine {
  class Feature_MeshBasePass : public FeatureRenderer {
  public:
    Feature_MeshBasePass(
      AssetManager *pAssets,
      GBuffer * pGBuffer,
      StructuredHardwareBuffer<renderer::ModelBuffer> * pModelBuffer)
      : m_pGBuffer(pGBuffer)
      , m_pModelData(pModelBuffer)
      , m_shader(pAssets, URI::File("engine:shaders/gbuffer/base.shader")) {
    }

    virtual void onAdded(Renderer * pRenderer) override {}

    virtual void renderView(Renderer * pRenderer, RenderView const & view) override {
      DeferredRenderer * pDeferred = (DeferredRenderer *)pRenderer;
      Mat4d              vp        = view.projectionMatrix * view.viewMatrix;

      graphics::StateManager * pState = pRenderer->getGraphicsDevice()->getStateManager();
      pState->setFeatureEnabled(GraphicsState_DepthTest, true);
      pState->setFeatureEnabled(GraphicsState_DepthWrite, true);
      pState->setFeatureEnabled(GraphicsState_Blend, false);

      GraphicsDevice * pDevice = pRenderer->getGraphicsDevice();
      pDevice->bindRenderTarget(m_pGBuffer->getRenderTarget());
      pDevice->clear({0, 0, 0, 0});
      pDevice->bindProgram(*m_shader);
      pDevice->bindUniformBuffer(*m_pModelData, renderer::BufferBinding_ModelBuffer);

      GraphicsResource defaultMaterial = pDeferred->getDefaultMaterial().getResource();

      geometry::Frustum<float> camFrustum = view.projectionMatrix * view.viewMatrix;

      for (auto & renderable : view.pRenderData->renderables<MeshRenderable>()) {
        if (!geometry::intersects(camFrustum, renderable.bounds)) {
          continue;
        }

        m_pModelData->data.modelMatrix  = renderable.modelMatrix;
        m_pModelData->data.normalMatrix = renderable.normalMatrix;
        m_pModelData->data.mvpMatrix    = vp * renderable.modelMatrix;
        m_pModelData->upload(pRenderer->getGraphicsDevice());

        for (auto & [i, texture] : enumerate(renderable.materialTextures)) {
          if (texture != InvalidGraphicsResource) {
            pDevice->bindTexture(texture, Material::TextureBindPointBase + i);
          } else {
            pDevice->bindTexture(pDeferred->getDefaultTexture((Material::TextureSlot)i).getResource(), Material::TextureBindPointBase + i);
          }
        }

        if (renderable.materialBuffer != InvalidGraphicsResource) {
          pDevice->bindUniformBuffer(renderable.materialBuffer, renderer::BufferBinding_PBRMaterial);
        } else {
          pDevice->bindUniformBuffer(defaultMaterial, renderer::BufferBinding_PBRMaterial);
        }

        pDevice->bindVertexArray(renderable.vertexArray);
        pDevice->drawIndexed(renderable.elementCount, renderable.elementOffset, PrimitiveType_Triangle);
      }

      pDevice->bindRenderTarget(view.renderTarget);
    }

    GBuffer * m_pGBuffer = nullptr;
    Asset<Shader> m_shader;

    StructuredHardwareBuffer<renderer::ModelBuffer> * m_pModelData = nullptr;
  };

  class Feature_LightingPass : public FeatureRenderer {
  public:
    // Data needed to render a shadow map
    struct ShadowMapData {
      Mat4                     lightVP;
      geometry::Frustum<float> lightFrustum;

      int64_t     lightIndex = 0;
      CubeMapFace cubeFace   = CubeMapFace_None; // Cube map face for point lights
      int64_t     atlasIndex = -1;

      LightRenderable light;
      float           maxDistance = 0.0f; // max distance the light will influence geometry from

      Vector<MeshShadowCasterRenderable> meshCasters;
    };

    Feature_LightingPass(GraphicsDevice * pDevice, AssetManager * pAssets, GBuffer * pGBuffer, GraphicsResource * pColourTarget,
                         StructuredHardwareBuffer<renderer::ModelBuffer> * pModelBuffer)
      : m_pGBuffer(pGBuffer)
      , m_pColourTarget(pColourTarget)
      , m_pModelData(pModelBuffer)
      , m_shadowAtlas(pDevice)
      , m_lightData(BufferUsageHint_Storage | BufferUsageHint_Dynamic)
      , m_shadowMaps(BufferUsageHint_Dynamic)
      , m_depthPass(pAssets, URI::File("engine:shaders/general/depth-pass.shader"))
      , m_shader(pAssets, URI::File("engine:shaders/pbr/lighting.shader")) {}

    virtual void onAdded(Renderer * pRenderer) override {
      // Setup shadow map render target
      GraphicsDevice * pDevice = pRenderer->getGraphicsDevice();
      m_shadowMapTarget        = {pDevice, pDevice->getRenderTargetManager()->createRenderTarget(RenderTargetType_Texture)};
    }

    virtual void beginView(Renderer * pRenderer, RenderView const & view) override {
      GraphicsDevice * pDevice = pRenderer->getGraphicsDevice();

      m_lightData.data.clear();
      m_shadowMaps.data.clear();

      // Free space allocated in shadow texture
      for (int64_t i = m_shadowMapData.size() - 1; i >= 0; --i) {
        m_shadowAtlas.release(m_shadowMapData[i].atlasIndex);
      }
      m_shadowMapData.clear();

      Mat4                     vp = view.projectionMatrix * view.viewMatrix;
      geometry::Frustum<float> camFrustum(vp);

      // Get light renderables in the views render data.
      for (LightRenderable const & light : view.pRenderData->renderables<LightRenderable>()) {
        renderer::LightBuffer item;
        item.type           = light.type;
        item.colour         = light.colour;
        item.ambient        = light.ambient;
        item.attenuation    = light.attenuation;
        item.direction      = light.direction;
        item.position       = light.position;
        item.strength       = light.strength;
        item.outerCutoff    = glm::cos(light.outerConeAngle);
        item.innerCutoff    = glm::cos(light.innerConeAngle);
        item.shadowMapRange = {0, 0};

        if (light.castShadows) {
          constexpr float limit = 0.01f;
          ShadowMapData   shadowMapData;
          shadowMapData.light      = light;
          shadowMapData.lightIndex = m_lightData.data.size();
          shadowMapData.maxDistance =
            math::maxComponent(math::solveQuadratic(limit * item.attenuation.z, limit * item.attenuation.y, limit * item.attenuation.x - item.strength));
          if (light.type == components::LightType_Point) {
            // add 6 entries for cube-map shadow texture
            for (int64_t i = 0; i < 6; ++i) {
              shadowMapData.cubeFace = (CubeMapFace)i;
              m_shadowMapData.pushBack(shadowMapData);
            }
          } else {
            m_shadowMapData.pushBack(shadowMapData);
          }
        }
        m_lightData.data.pushBack(item);
      }

      // Add a default light if none are in the render data.
      if (m_lightData.data.size() == 0) {
        renderer::LightBuffer l;
        l.type      = components::LightType_Sun;
        l.direction = Vec3(1, 0, 0);
        l.ambient   = Vec3(1);
        l.colour    = Vec3(0);
        l.strength  = 1;
        m_lightData.data.pushBack(l);
      }

      if (m_shadowMapData.size() > 0) {
        geometry::Boxf receiverBounds;
        for (auto & receiver : view.pRenderData->renderables<MeshRenderable>()) {
          if (receiver.bounds.invalid()) {
            continue;
          }

          if (geometry::intersects(camFrustum, receiver.bounds)) {
            receiverBounds.growToContain(receiver.bounds);
          }
        }

        // Calculate vp and renderables to draw
        for (auto & [i, shadowMapData] : enumerate(m_shadowMapData)) {
          calcShadowMapData(view.pRenderData, receiverBounds, &shadowMapData);
        }

        // Allocate space in the shadow texture
        for (ShadowMapData & shadowMapData : m_shadowMapData) {
          if (shadowMapData.meshCasters.size() > 0) {
            shadowMapData.atlasIndex = m_shadowAtlas.allocate(1.0f);
          }
        }
        // Reslove allocations
        m_shadowAtlas.pack();

        int64_t                 lightIndex = -1;
        renderer::LightBuffer * pCurLight  = nullptr;
        for (ShadowMapData const & shadowMapData : m_shadowMapData) {
          if (shadowMapData.lightIndex != lightIndex) {
            lightIndex = shadowMapData.lightIndex;
            pCurLight  = m_lightData.data.begin() + lightIndex;

            pCurLight->shadowMapRange.x = (uint32_t)m_shadowMaps.data.size();
          }

          renderer::ShadowMapBuffer shadowMap;
          if (shadowMapData.atlasIndex != -1) {
            ShadowAtlas::Slot slot = m_shadowAtlas.getSlot(shadowMapData.atlasIndex);
            shadowMap.layer        = slot.layer;
            shadowMap.level        = slot.level;
          } else {
            shadowMap.layer = -1;
            shadowMap.level = -1;
          }

          shadowMap.lightVP = shadowMapData.lightVP;
          m_shadowMaps.data.pushBack(shadowMap);

          if (pCurLight != nullptr) {
            pCurLight->shadowMapRange.y = (uint32_t)m_shadowMaps.data.size();
          }
        }

        m_shadowMaps.upload(pDevice);
      }
      m_lightData.upload(pDevice);
    }

    virtual void renderView(Renderer * pRenderer, RenderView const & view) override {
      GraphicsDevice *                pDevice = pRenderer->getGraphicsDevice();
      graphics::RenderTargetManager * pRT     = pDevice->getRenderTargetManager();
      graphics::StateManager *        pState  = pDevice->getStateManager();

      /// Render shadow maps
      if (m_shadowMapData.size() > 0) {
        pDevice->bindProgram(*m_depthPass);

        pState->setFeatureEnabled(GraphicsState_DepthTest, true);
        pState->setFeatureEnabled(GraphicsState_DepthWrite, true);
        pState->setColourWriteEnabled(false, false, false, false);

        for (ShadowMapData const & shadowMapData : m_shadowMapData) {
          if (shadowMapData.meshCasters.size() == 0) {
            continue;
          }

          ShadowAtlas::Slot slot = m_shadowAtlas.getSlot(shadowMapData.atlasIndex);
          pRT->attachDepth(m_shadowMapTarget, m_shadowAtlas, slot.level, slot.layer);
          pDevice->bindRenderTarget(m_shadowMapTarget);
          pDevice->clear(0);
          pState->setViewport({0, 0}, m_shadowAtlas.resolution(shadowMapData.atlasIndex));

          for (MeshShadowCasterRenderable const & caster : view.pRenderData->renderables<MeshShadowCasterRenderable>()) {
            if (geometry::intersects(shadowMapData.lightFrustum, caster.bounds)) {
              m_pModelData->data.mvpMatrix = (Mat4d)shadowMapData.lightVP * caster.modelMatrix;
              m_pModelData->upload(pDevice);

              pDevice->bindVertexArray(caster.vertexArray);
              pDevice->drawIndexed(caster.elementCount, caster.elementOffset);
            }
          }
        }

        pState->setColourWriteEnabled(true, true, true, true);
      }

      /// Find final scene colour texture.
      /// This target accumulates post-processing effects
      pDevice->bindRenderTarget(*m_pColourTarget);
      pState->setViewport({0, 0}, pRT->getSize(*m_pColourTarget));
      pState->setFeatureEnabled(GraphicsState_DepthTest, false);
      pState->setFeatureEnabled(GraphicsState_DepthWrite, false);
      pState->setFeatureEnabled(GraphicsState_StencilTest, false);
      pState->setFeatureEnabled(GraphicsState_Blend, false);

      // Bind post-process texture inputs.
      // TODO: Generalize this using a PostProcessInputRenderable/GBufferInputRenderable?
      for (auto & [i, tex] : enumerate(*m_pGBuffer)) {
        pDevice->bindTexture(tex.getResource(), DeferredRenderer::ColourTargetBindPointBase + i);
      }
      pDevice->bindTexture(m_shadowAtlas, DeferredRenderer::ColourTargetBindPointBase + GBufferTarget_Count);
      pDevice->bindShaderStorageBuffer(m_shadowMaps, 3);

      pDevice->bindProgram(*m_shader);
      pDevice->bindVertexArray(InvalidGraphicsResource);
      pDevice->bindShaderStorageBuffer(m_lightData, renderer::BufferBinding_LightBuffer);
      pDevice->draw(3);
      pDevice->bindRenderTarget(view.renderTarget);
    }

    static void calcShadowMapData(RenderData * pRenderData, geometry::Boxf const & receiverBounds, ShadowMapData * pData) {
      switch (pData->light.type) {
      case components::LightType_Sun: calcShadowMapDataForSun(pRenderData, receiverBounds, pData); break;
      case components::LightType_Point: calcShadowMapDataForPointLight(pRenderData, pData); break;
      case components::LightType_Spot: calcShadowMapDataForSpotLight(pRenderData, pData); break;
      default: break;
      }
    }

    static void calcShadowMapDataForSun(RenderData * pRenderData, geometry::Boxf const & receiverBounds, ShadowMapData * pData) {
      auto &                  allCasters = pRenderData->renderables<MeshShadowCasterRenderable>();
      LightRenderable const & light      = pData->light;

      // Calculate light axis
      Vec3 right, up;
      math::calculateAxes(light.direction, &up, &right);

      // Get receiver bounds in light space
      geometry::Boxf lsReceiverBounds = receiverBounds.projected(right, up, light.direction);
      lsReceiverBounds.min.z          = -std::numeric_limits<float>::max();

      // Clear old data
      pData->meshCasters.clear();

      // Get casters between the light and the receivers
      geometry::Boxf lsAllCasterBounds;
      for (auto & caster : allCasters) {
        geometry::Boxf lsCasterBounds = caster.bounds.projected(right, up, light.direction);
        if (geometry::intersects(lsReceiverBounds, lsCasterBounds)) {
          lsAllCasterBounds.growToContain(lsCasterBounds);
          pData->meshCasters.pushBack(caster);
        }
      }

      // Calculate orthographic projection that encompasses all casters
      Mat4 projection = glm::ortho(lsAllCasterBounds.min.x, lsAllCasterBounds.max.x, lsAllCasterBounds.min.y, lsAllCasterBounds.max.y,
                                   lsAllCasterBounds.min.z * 2, lsAllCasterBounds.max.z * 2);
      Mat4 view       = glm::lookAt(Vec3(0), light.direction, up);

      pData->lightVP      = projection * view;
      pData->lightFrustum = pData->lightVP;
    }

    static void calcShadowMapDataForPointLight(RenderData * pRenderData, ShadowMapData * pData) {
      LightRenderable const & light = pData->light;

      Vec3  direction = (Vec3)getCubeMapDirection(pData->cubeFace);
      Vec3  up        = (Vec3)getCubeMapUp(pData->cubeFace);
      float dist      = pData->maxDistance;

      // Calculate view-projection for the target face of the cube map
      Mat4 projection = glm::perspective(math::radians(90.0f), 1.0f, dist * 0.01f, dist);
      Mat4 view       = glm::lookAt(light.position, light.position + direction, up);

      pData->lightVP      = projection * view;
      pData->lightFrustum = pData->lightVP;

      // Clear old data
      pData->meshCasters.clear();

      // Add casters for this light
      for (MeshShadowCasterRenderable const & caster : pRenderData->renderables<MeshShadowCasterRenderable>()) {
        if (geometry::intersects(pData->lightFrustum, caster.bounds)) {
          pData->meshCasters.pushBack(caster);
        }
      }
    }

    static void calcShadowMapDataForSpotLight(RenderData * pRenderData, ShadowMapData * pData) {
      LightRenderable const & light = pData->light;

      // Calculate light axis
      Vec3 right, up;
      math::calculateAxes(light.direction, &up, &right);

      // Calculate view-projection for the spot light
      float dist       = pData->maxDistance;
      Mat4  projection = glm::perspective(light.outerConeAngle * 2, 1.0f, 0.01f * dist, dist);
      Mat4  view       = glm::lookAt(light.position, light.position + light.direction, up);

      pData->lightVP      = projection * view;
      pData->lightFrustum = pData->lightVP;

      // Clear old data
      pData->meshCasters.clear();

      // Add casters for this light
      for (MeshShadowCasterRenderable const & caster : pRenderData->renderables<MeshShadowCasterRenderable>()) {
        if (geometry::intersects(pData->lightFrustum, caster.bounds)) {
          pData->meshCasters.pushBack(caster);
        }
      }
    }

    // Shadow mapping
    ManagedGraphicsResource m_shadowMapTarget;
    ShadowAtlas             m_shadowAtlas;

    Asset<Shader> m_depthPass;

    Vector<ShadowMapData>                                    m_shadowMapData; // Data rendered per shadow map
    StructuredHardwareArrayBuffer<renderer::ShadowMapBuffer> m_shadowMaps;
    StructuredHardwareBuffer<renderer::ModelBuffer> *        m_pModelData = nullptr;

    // Lighting pass
    GBuffer *          m_pGBuffer = nullptr;
    Asset<Shader>      m_shader;
    GraphicsResource * m_pColourTarget = nullptr;

    StructuredHardwareArrayBuffer<renderer::LightBuffer> m_lightData;
  };

  class Feature_Skybox : public FeatureRenderer {
  public:
    Feature_Skybox(AssetManager * pAssets, GraphicsResource * pColourTarget)
      : m_shader(pAssets, URI::File("engine:shaders/general/cubemap.shader"))
      , m_pColourTarget(pColourTarget) {}

    virtual void renderView(Renderer * pRenderer, RenderView const & view) override {
      const auto &                    skyboxes = view.pRenderData->renderables<CubeMapRenderable>();
      if (skyboxes.size() == 0) {
        return;
      }

      auto pDevice = pRenderer->getGraphicsDevice();
      auto                            pState  = pDevice->getStateManager();
      graphics::RenderTargetManager * pRT     = pDevice->getRenderTargetManager();

      /// Bind final scene colour texture.
      /// This target accumulates post-processing effects
      pDevice->bindRenderTarget(*m_pColourTarget);
      pState->setViewport({0, 0}, pRT->getSize(*m_pColourTarget));
      pState->setFeatureEnabled(GraphicsState_DepthTest, true);
      pState->setFeatureEnabled(GraphicsState_DepthWrite, false);
      pState->setDepthFunction(ComparisonFunction_Equal);
      pState->setFeatureEnabled(GraphicsState_StencilTest, false);
      pState->setFeatureEnabled(GraphicsState_Blend, false);
      pState->setDepthRange(1, 1);

      pDevice->bindProgram(*m_shader);
      pDevice->bindVertexArray(InvalidGraphicsResource);
      for (auto const & cm : skyboxes) {
        pDevice->bindTexture(cm.texture, 0);
      }
      pDevice->draw(6);
      pDevice->bindRenderTarget(view.renderTarget);

      pState->setDepthRange(0, 1);
      pState->setDepthFunction(ComparisonFunction_Less);
    }

    Asset<Shader>      m_shader;
    GraphicsResource * m_pColourTarget = nullptr;
  };

  class Feature_SSAO : public FeatureRenderer {
  public:
    Feature_SSAO(AssetManager * pAssets, PostProcessingStack * pPPS)
      : m_pPPS(pPPS)
      , m_ssaoShader(pAssets, URI::File("engine:shaders/ssao/ssao.shader")) {
    }

    virtual void onAdded(Renderer * pRenderer) override {
      GraphicsDevice *                pDevice   = pRenderer->getGraphicsDevice();
      graphics::RenderTargetManager * pRT       = pDevice->getRenderTargetManager();
      graphics::TextureManager *      pTextures = pDevice->getTextureManager();

      m_sampleKernel.resize(m_kernelSize);
      for (int64_t i = 0; i < m_kernelSize; ++i) {
        m_sampleKernel[i] = glm::ballRand(1.0f) * glm::lerp(0.1f, 1.0f, math::pow<2>(i / (float)m_kernelSize));
      }

      Vector<Colour<RGBu8>> randData;
      randData.resize(m_randSize * m_randSize);
      for (Colour<RGBu8> & c : randData) {
        Vec3 v = glm::sphericalRand(1.0f) * 0.5f + 0.5f;
        c.r    = uint8_t(v.x * 255.0f);
        c.g    = uint8_t(v.y * 255.0f);
        c.b    = uint8_t(v.z * 255.0f);
      }
      m_randomTex.load2D(pDevice, {m_randSize, m_randSize}, randData.data());
    }

    virtual void beginView(Renderer * pRenderer, RenderView const & view) override {
      auto & ssaoOpts = view.pRenderData->renderables<PostProcessRenderable_SSAO>();
      if (ssaoOpts.size() == 0)
        return;

      PostProcessRenderable_SSAO & ssao = ssaoOpts.front();

      float bias     = ssao.bias;
      float radius   = ssao.radius;
      float strength = ssao.strength;

      m_pPPS->addPass([=](GraphicsDevice * pDevice, PostProcessParams const & params) {
        graphics::RenderTargetManager * pRT    = pDevice->getRenderTargetManager();
        graphics::StateManager *        pState = pDevice->getStateManager();
        params.bindInputs(pDevice);
        params.bindTarget(pDevice);
        pDevice->bindProgram(*m_ssaoShader);
        pDevice->bindTexture(m_randomTex, 7);
        m_ssaoShader->setUniform("radius", radius);
        m_ssaoShader->setUniform("strength", strength);
        m_ssaoShader->setUniform("bias", bias);
        m_ssaoShader->setUniform("outputSize", params.sceneColour.getSize());

        for (int64_t i = 0; i < m_sampleKernel.size(); ++i) {
          m_ssaoShader->setUniform(String::format("sampleKernel[%lld]", i), m_sampleKernel[i]);
        }

        pDevice->bindVertexArray(InvalidGraphicsResource);
        pDevice->draw(3);
      });
    }

    Asset<Shader>         m_ssaoShader;
    Texture               m_randomTex;
    Vector<Vec3>          m_sampleKernel;
    int64_t               m_kernelSize = 32;
    int64_t               m_randSize   = 4;
    PostProcessingStack * m_pPPS       = nullptr;
  };

  class Feature_SSR : public FeatureRenderer {
  public:
    Feature_SSR(AssetManager * pAssets, PostProcessingStack * pPPS)
      : m_pPPS(pPPS) {
      programs.calculateReflections.assign(pAssets, URI::File("engine:shaders/ssr/calculate"));
      programs.blendReflections.assign(pAssets, URI::File("engine:shaders/ssr/blend"));
      programs.upsampler.assign(pAssets, URI::File("engine:shaders/bloom/upsample"));
      programs.downsampler.assign(pAssets, URI::File("engine:shaders/bloom/downsample"));
    }

    virtual void onAdded(Renderer * pRenderer) override {
      GraphicsDevice *                pDevice   = pRenderer->getGraphicsDevice();
      graphics::RenderTargetManager * pRT       = pDevice->getRenderTargetManager();
      graphics::TextureManager *      pTextures = pDevice->getTextureManager();

      m_clampSampler     = {pDevice, pTextures->createSampler()};
      m_mipChainTarget   = {pDevice, pRT->createRenderTarget(RenderTargetType_Texture)};
      m_reflectionTarget = {pDevice, pRT->createRenderTarget(RenderTargetType_Texture)};

      pTextures->setSamplerWrap(m_clampSampler, WrapMode_ClampToEdge);
    }

    virtual void beginView(Renderer * pRenderer, RenderView const & view) override {
      auto & ssrOpt = view.pRenderData->renderables<PostProcessRenderable_SSR>();
      if (ssrOpt.size() == 0)
        return;

      GraphicsDevice *                pDevice = pRenderer->getGraphicsDevice();
      graphics::RenderTargetManager * pRT     = pDevice->getRenderTargetManager();

      Vec2i size = pRT->getSize(view.renderTarget);
      if (size != m_chainSize) {
        m_reflectionUV.load2D(pDevice, size, PixelFormat_RGBAf16);
        pRT->attachColour(m_reflectionTarget, m_reflectionUV);

        m_mipChain.resize(m_mipChainSize);
        m_chainSize     = size;
        Vec2i levelSize = size;
        for (int64_t i = 0; i < m_mipChainSize; ++i) {
          m_mipChain[i].load2D(pDevice, levelSize, PixelFormat_RGBAf16);
          levelSize /= 2;
        }
      }

      auto & opt         = ssrOpt.front();
      float  maxDistance = opt.maxDistance;
      float  resolution  = opt.resolution;
      int    steps       = opt.steps;
      float  thickness   = opt.thickness;

      m_pPPS->addPass([=](GraphicsDevice * pDevice, PostProcessParams const & params) {
        graphics::RenderTargetManager * pRT    = pDevice->getRenderTargetManager();
        graphics::StateManager *        pState = pDevice->getStateManager();

        Vec2i targetSize = pRT->getSize(params.target);

        // Calculate reflections
        params.bindInputs(pDevice);
        pDevice->bindRenderTarget(m_reflectionTarget);
        pState->setViewport({0, 0}, m_mipChain[0].getSize());
        pDevice->bindProgram(*programs.calculateReflections);
        programs.calculateReflections->setUniform("maxDistance", maxDistance);
        programs.calculateReflections->setUniform("resolution", resolution);
        programs.calculateReflections->setUniform("steps", steps);
        programs.calculateReflections->setUniform("thickness", thickness);
        programs.calculateReflections->setUniform("outputSize", targetSize);

        params.bindInputs(pDevice);
        pDevice->bindVertexArray(InvalidGraphicsResource);
        pDevice->draw(3);

        // Blur reflections (for roughness)
        pDevice->bindProgram(*programs.downsampler);
        pDevice->bindVertexArray(InvalidGraphicsResource);
        pDevice->bindSampler(m_clampSampler, 0);
        pDevice->bindTexture(params.sceneColour, 0);
        for (int64_t i = 1; i < m_mipChain.size(); ++i) {
          pRT->attachColour(m_mipChainTarget, m_mipChain[i]);
          pDevice->bindRenderTarget(m_mipChainTarget);
          pState->setViewport({0, 0}, m_mipChain[i].getSize());
          pDevice->draw(3);
          pDevice->bindTexture(m_mipChain[i], 0);
        }

        // Blend reflections with final output
        params.bindInputs(pDevice);
        params.bindTarget(pDevice);
        pDevice->bindProgram(*programs.blendReflections);
        pDevice->bindTexture(m_reflectionUV, 7);
        pDevice->bindTexture(m_mipChain.back(), 8);
        pDevice->bindTexture(m_brdfLUT, 9);
        pDevice->draw(3);
      });
    }

  private:
    ManagedGraphicsResource m_clampSampler;

    struct {
      Asset<Shader> calculateReflections;
      Asset<Shader> blendReflections;
      Asset<Shader> upsampler;
      Asset<Shader> downsampler;
    } programs;

    int64_t m_mipChainSize = 7;

    Texture m_brdfLUT; // TODO: Calculate brdf LUT

    Texture                 m_reflectionUV;
    ManagedGraphicsResource m_reflectionTarget;

    ManagedGraphicsResource m_mipChainTarget;
    Vec2i                   m_chainSize;
    Vector<Texture>         m_mipChain;

    PostProcessingStack * m_pPPS = nullptr;
  };

  class Feature_Bloom : public FeatureRenderer {
  public:
    Feature_Bloom(AssetManager * pAssets, PostProcessingStack * pPPS, int64_t mipChainSize = 5)
      : m_pPPS(pPPS)
      , m_upsampler(pAssets, URI::File("engine:shaders/bloom/upsample"))
      , m_downsampler(pAssets, URI::File("engine:shaders/bloom/downsample"))
      , m_blendBloom(pAssets, URI::File("engine:shaders/bloom/mix"))
      , m_prefilter(pAssets, URI::File("engine:shaders/bloom/prefilter")) {
      m_mipChain.resize(mipChainSize);
      m_mipChainSize = mipChainSize;
    }

    virtual void onAdded(Renderer * pRenderer) override {
      GraphicsDevice *                pDevice   = pRenderer->getGraphicsDevice();
      graphics::RenderTargetManager * pRT       = pDevice->getRenderTargetManager();
      graphics::TextureManager *      pTextures = pDevice->getTextureManager();

      m_mipChainTarget = {pDevice, pRT->createRenderTarget(RenderTargetType_Texture)};
      m_filteredTarget = {pDevice, pRT->createRenderTarget(RenderTargetType_Texture)};
      m_clampSampler   = {pDevice, pTextures->createSampler()};
      pTextures->setSamplerWrap(m_clampSampler, WrapMode_ClampToEdge);
    }

    virtual void onResize(Renderer * pRenderer, Vec2i const & size) override {}

    virtual void beginFrame(Renderer * pRenderer, Vector<RenderView> const & views) {
    }

    virtual void beginView(Renderer * pRenderer, RenderView const & view) override {
      GraphicsDevice *                pDevice = pRenderer->getGraphicsDevice();
      graphics::RenderTargetManager * pRT     = pDevice->getRenderTargetManager();
      Vec2i                           size    = pRT->getSize(view.renderTarget);
      if (size != m_chainSize) {
        m_mipChain.resize(m_mipChainSize);
        m_chainSize     = size;
        Vec2i levelSize = size;
        for (int64_t i = 0; i < m_mipChainSize; ++i) {
          levelSize /= 2;
          m_mipChain[i].load2D(pDevice, levelSize, PixelFormat_RGBAf16);
        }
        m_filtered.load2D(pDevice, size, PixelFormat_RGBAf16);
        pRT->attachColour(m_filteredTarget, m_filtered);
      }

      auto & bloomOpts = view.pRenderData->renderables<PostProcessRenderable_Bloom>();
      if (bloomOpts.size() == 0)
        return;
      float            filterRadius  = bloomOpts.front().filterRadius;
      float            strength      = bloomOpts.front().strength;
      float            threshold     = bloomOpts.front().threshold;
      float            dirtIntensity = bloomOpts.front().dirtIntensity;
      GraphicsResource dirtTex       = bloomOpts.front().dirtTex;

      m_pPPS->addPass([=](GraphicsDevice * pDevice, PostProcessParams const & params) {
        graphics::RenderTargetManager * pRT    = pDevice->getRenderTargetManager();
        graphics::StateManager *        pState = pDevice->getStateManager();

        if (threshold > 0) {
          pDevice->bindProgram(*m_prefilter);
          m_prefilter->setUniform("threshold", threshold);
          pDevice->bindRenderTarget(m_filteredTarget);
          pState->setViewport({0, 0}, pRT->getSize(m_filteredTarget));
          pDevice->bindTexture(params.sceneColour, 0);
          pDevice->draw(3);
          pDevice->bindTexture(m_filtered, 0);
        } else {
          pDevice->bindTexture(params.sceneColour, 0);
        }

        // Down-sample the hdr colour texture
        pDevice->bindProgram(*m_downsampler);
        pDevice->bindVertexArray(InvalidGraphicsResource);
        pDevice->bindSampler(m_clampSampler, 0);
        for (int64_t i = 0; i < m_mipChain.size(); ++i) {
          pRT->attachColour(m_mipChainTarget, m_mipChain[i]);
          pDevice->bindRenderTarget(m_mipChainTarget);
          pState->setViewport({0, 0}, m_mipChain[i].getSize());
          pDevice->draw(3);
          pDevice->bindTexture(m_mipChain[i], 0);
        }

        // Up-sample down-sampled colour texture
        pDevice->bindProgram(*m_upsampler);
        m_upsampler->setUniform("filterRadius", filterRadius);

        pState->setFeatureEnabled(GraphicsState_Blend, true);
        pState->setBlendEquation(BlendEquation_Add);
        pState->setBlendFunction(BlendFunction_One, BlendFunction_One);
        pDevice->bindVertexArray(InvalidGraphicsResource);
        for (int64_t i = m_mipChain.size() - 1; i > 0; --i) {
          pRT->attachColour(m_mipChainTarget, m_mipChain[i - 1]);
          pDevice->bindRenderTarget(m_mipChainTarget);
          pState->setViewport({0, 0}, m_mipChain[i - 1].getSize());
          pDevice->bindTexture(m_mipChain[i], 0);
          pDevice->draw(3);
        }

        pState->setFeatureEnabled(GraphicsState_Blend, false);
        pState->setBlendEquation(BlendEquation_Add);
        pState->setBlendFunction(BlendFunction_SourceAlpha, BlendFunction_OneMinusSourceAlpha);

        // Bind final render target
        params.bindTarget(pDevice);
        params.bindInputs(pDevice);

        // Mix bloom with source colour
        pDevice->bindProgram(*m_blendBloom);
        m_blendBloom->setUniform("strength", strength);
        m_blendBloom->setUniform("dirtIntensity", dirtIntensity);
        pDevice->bindTexture(params.sceneColour, 0);
        pDevice->bindTexture(m_mipChain.front(), 1);
        pDevice->bindTexture(dirtTex, 2);
        pDevice->draw(3);
        pDevice->bindSampler(InvalidGraphicsResource, 0);
      });
    }

    PostProcessingStack * m_pPPS = nullptr;

    Asset<Shader> m_upsampler;
    Asset<Shader> m_downsampler;
    Asset<Shader> m_blendBloom;
    Asset<Shader> m_prefilter;

    ManagedGraphicsResource m_clampSampler;

    int64_t m_mipChainSize = 5;

    ManagedGraphicsResource m_mipChainTarget;
    Vec2i                   m_chainSize;
    Vector<Texture>         m_mipChain;

    ManagedGraphicsResource m_filteredTarget;
    Texture                 m_filtered;
  };

  class Feature_Exposure : public FeatureRenderer {
  public:
    Feature_Exposure(AssetManager * pAssets, PostProcessingStack * pPPS)
      : m_pPPS(pPPS)
      , m_shader(pAssets, URI::File("engine:shaders/simple-tonemapper.shader")) {}

    virtual void beginView(Renderer * pRenderer, RenderView const & view) override {
      GraphicsDevice * pDevice = pRenderer->getGraphicsDevice();

      auto  exposureRenderables = view.pRenderData->renderables<PostProcessRenderable_Exposure>();
      float exposure            = 1.0f;
      if (exposureRenderables.size() > 0)
        exposure = exposureRenderables.front().exposure;

      m_pPPS->addPass([=](GraphicsDevice * pDevice, PostProcessParams const & params) {
        // Currently only support tone-mapping post process effect.
        // TODO: Add support for more effects (maybe custom effects as well?)
        params.bindInputs(pDevice);
        params.bindTarget(pDevice);

        pDevice->bindProgram(*m_shader);

        m_shader->setUniform("exposure", exposure);

        pDevice->bindVertexArray(InvalidGraphicsResource);
        pDevice->draw(3);
      });
    }

    Asset<Shader>         m_shader;
    PostProcessingStack * m_pPPS = nullptr;
  };

  class Feature_PostProcess : public FeatureRenderer {
  public:
    Feature_PostProcess(AssetManager * pAssets, PostProcessingStack * pPPS, GBuffer * pGBuffer, Texture * pFinalColour)
      : m_pPPS(pPPS)
      , m_pGBuffer(pGBuffer)
      , m_pFinalColour(pFinalColour) {}

    virtual void renderView(Renderer * pRenderer, RenderView const & view) override {
      m_pPPS->target               = view.renderTarget;
      m_pPPS->sceneColour          = *m_pFinalColour;
      m_pPPS->inputs.sceneDepth    = m_pGBuffer->getDepthTarget();
      m_pPPS->inputs.baseColour    = m_pGBuffer->getBaseColour();
      m_pPPS->inputs.ambientColour = m_pGBuffer->getAmbientColour();
      m_pPPS->inputs.normal        = m_pGBuffer->getNormal();
      m_pPPS->inputs.position      = m_pGBuffer->getPosition();
      m_pPPS->inputs.rma           = m_pGBuffer->getRMA();

      GraphicsDevice *                pDevice        = pRenderer->getGraphicsDevice();
      graphics::RenderTargetManager * pRenderTargets = pDevice->getRenderTargetManager();
      m_pPPS->execute(pDevice, pRenderTargets->getSize(view.renderTarget));
    }

    PostProcessingStack * m_pPPS         = nullptr;
    GBuffer *             m_pGBuffer     = nullptr;
    Texture *             m_pFinalColour = nullptr;
  };

  DeferredRenderer::DeferredRenderer(GraphicsDevice * pDevice, AssetManager * pAssets)
    : Renderer(pDevice)
    , m_gbuffer(pDevice, {1920, 1080})
    , m_pDevice(pDevice)
    , m_modelData(BufferUsageHint_Uniform | BufferUsageHint_Dynamic)
    , m_cameraData(BufferUsageHint_Uniform | BufferUsageHint_Dynamic) {
    // Create renderer resources
    m_finalColourTarget.load2D(m_pDevice, {1920, 1080}, PixelFormat_RGBAf16);

    // Attach final colour texture to the final target
    graphics::RenderTargetManager * pRenderTargets = m_pDevice->getRenderTargetManager();

    m_finalTarget = pRenderTargets->createRenderTarget(RenderTargetType_Texture);
    pRenderTargets->attachColour(m_finalTarget, m_finalColourTarget);
    pRenderTargets->attachDepth(m_finalTarget, m_gbuffer.getDepthTarget(), 0, 0);

    Colour<RGBAu8> white[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    Colour<RGBAu8> black[4] = {0, 0, 0, 0};
    Colour<RGBAu8> blue[4]  = {{127, 127, 255, 1}, {127, 127, 255, 1}, {127, 127, 255, 1}, {127, 127, 255, 1}};

    // Load some default textures
    Texture whiteTex, blackTex, blueTex;
    whiteTex.load2D(m_pDevice, {2, 2}, white);
    blackTex.load2D(m_pDevice, {2, 2}, black);
    blueTex.load2D(m_pDevice, {2, 2}, blue);

    // Assign default textures for each material slot
    m_defaultMaterialTextures[Material::TextureSlot_Alpha]      = whiteTex;
    m_defaultMaterialTextures[Material::TextureSlot_BaseColour] = whiteTex;
    m_defaultMaterialTextures[Material::TextureSlot_Ambient]    = blackTex;
    m_defaultMaterialTextures[Material::TextureSlot_Emissive]   = blackTex;
    m_defaultMaterialTextures[Material::TextureSlot_Roughness]  = whiteTex;
    m_defaultMaterialTextures[Material::TextureSlot_Metalness]  = whiteTex;
    m_defaultMaterialTextures[Material::TextureSlot_AO]         = whiteTex;
    m_defaultMaterialTextures[Material::TextureSlot_Alpha]      = whiteTex;
    m_defaultMaterialTextures[Material::TextureSlot_Normal]     = blueTex;

    m_defaultMaterial.upload(pDevice);

    // Add renderer features
    addFeature<Feature_MeshBasePass>(pAssets, & m_gbuffer, &m_modelData);
    addFeature<Feature_LightingPass>(pDevice, pAssets, &m_gbuffer, &m_finalTarget, &m_modelData);
    addFeature<Feature_Skybox>(pAssets, &m_finalTarget);
    addFeature<Feature_SSAO>(pAssets, &m_postProc);
    addFeature<Feature_SSR>(pAssets, &m_postProc);
    addFeature<Feature_Bloom>(pAssets, &m_postProc);
    addFeature<Feature_Exposure>(pAssets, &m_postProc);
    addFeature<Feature_PostProcess>(pAssets, &m_postProc, &m_gbuffer, &m_finalColourTarget);
  }

  GraphicsResource DeferredRenderer::getFinalTarget() const {
    return m_finalTarget;
  }

  Texture const & DeferredRenderer::getDefaultTexture(Material::TextureSlot slot) const {
    return m_defaultMaterialTextures[slot];
  }

  bfc::StructuredHardwareBuffer<bfc::renderer::PBRMaterial> const & DeferredRenderer::getDefaultMaterial() const {
    return m_defaultMaterial;
  }

  void DeferredRenderer::onResize(Vec2i size) {
    Renderer::onResize(size);

    m_gbuffer.resize(size);
    m_finalColourTarget.load2D(m_pDevice, size, PixelFormat_RGBAf16);
  }

  void DeferredRenderer::render(Vector<RenderView> const & views) {
    graphics::RenderTargetManager * pRenderTargets = getGraphicsDevice()->getRenderTargetManager();

    getGraphicsDevice()->bindUniformBuffer(m_cameraData.getResource(), renderer::BufferBinding_CameraBuffer);
    getGraphicsDevice()->bindUniformBuffer(m_modelData.getResource(), renderer::BufferBinding_ModelBuffer);
    m_postProc.reset();

    Renderer::render(views);
  }

  void DeferredRenderer::beginView(RenderView const & view) {
    Mat4d dVP = view.projectionMatrix * view.viewMatrix;

    m_cameraData.data.viewProjMatrix    = dVP;
    m_cameraData.data.invViewProjMatrix = glm::inverse(dVP);
    m_cameraData.data.projMatrix        = view.projectionMatrix;
    m_cameraData.data.viewMatrix        = view.viewMatrix;
    m_cameraData.data.invProjMatrix     = glm::inverse(view.projectionMatrix);
    m_cameraData.data.invViewMatrix     = glm::inverse(view.viewMatrix);
    m_cameraData.upload(getGraphicsDevice());
  }

  void DeferredRenderer::endView(RenderView const & view) {
    BFC_UNUSED(view);
  }
} // namespace engine
