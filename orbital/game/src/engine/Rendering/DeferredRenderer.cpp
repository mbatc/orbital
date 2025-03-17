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
    Feature_MeshBasePass(AssetManager * pAssets, GBuffer * pGBuffer, graphics::StructuredBuffer<renderer::ModelBuffer> * pModelBuffer)
      : m_pGBuffer(pGBuffer)
      , m_pModelData(pModelBuffer)
      , m_shader(pAssets, URI::File("engine:shaders/gbuffer/base.shader")) {}

    virtual void onAdded(graphics::CommandList * pCmdList, Renderer * pRenderer) override {}

    virtual void renderView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) override {
      DeferredRenderer * pDeferred = (DeferredRenderer *)pRenderer;
      Mat4d              vp        = view.projectionMatrix * view.viewMatrix;

      graphics::StateManager * pState = pRenderer->getGraphicsDevice()->getStateManager();
      pCmdList->pushState(graphics::State::EnableDepthRead{true}, graphics::State::EnableDepthWrite{true}, graphics::State::EnableBlend{false});

      GraphicsDevice * pDevice = pRenderer->getGraphicsDevice();
      pCmdList->bindRenderTarget(m_pGBuffer->getRenderTarget());
      pCmdList->clear({0, 0, 0, 0});
      pCmdList->bindProgram(m_shader);
      pCmdList->bindUniformBuffer(*m_pModelData, renderer::BufferBinding_ModelBuffer);

      graphics::BufferRef defaultMaterial = pDeferred->getDefaultMaterial();

      geometry::Frustum<float> camFrustum = view.projectionMatrix * view.viewMatrix;

      for (auto & renderable : view.pRenderData->renderables<MeshRenderable>()) {
        if (!geometry::intersects(camFrustum, renderable.bounds)) {
          continue;
        }

        m_pModelData->data.modelMatrix  = renderable.modelMatrix;
        m_pModelData->data.normalMatrix = renderable.normalMatrix;
        m_pModelData->data.mvpMatrix    = vp * renderable.modelMatrix;
        m_pModelData->upload(pCmdList);

        for (auto & [i, texture] : enumerate(renderable.materialTextures)) {
          if (texture != InvalidGraphicsResource) {
            pCmdList->bindTexture(texture, Material::TextureBindPointBase + i);
          } else {
            pCmdList->bindTexture(pDeferred->getDefaultTexture((Material::TextureSlot)i), Material::TextureBindPointBase + i);
          }
        }

        if (renderable.materialBuffer != InvalidGraphicsResource) {
          pCmdList->bindUniformBuffer(renderable.materialBuffer, renderer::BufferBinding_PBRMaterial);
        } else {
          pCmdList->bindUniformBuffer(defaultMaterial, renderer::BufferBinding_PBRMaterial);
        }

        pCmdList->bindVertexArray(renderable.vertexArray);
        pCmdList->drawIndexed(renderable.elementCount, renderable.elementOffset, PrimitiveType_Triangle);
      }

      pCmdList->bindRenderTarget(view.renderTarget);
      pCmdList->popState();
    }

    GBuffer *     m_pGBuffer = nullptr;
    Asset<graphics::Program> m_shader;

    graphics::StructuredBuffer<renderer::ModelBuffer> * m_pModelData = nullptr;
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

    Feature_LightingPass(graphics::CommandList * pCmdList, AssetManager * pAssets, GBuffer * pGBuffer, graphics::RenderTargetRef * pColourTarget,
                         graphics::StructuredBuffer<renderer::ModelBuffer> * pModelBuffer)
      : m_pGBuffer(pGBuffer)
      , m_pColourTarget(pColourTarget)
      , m_pModelData(pModelBuffer)
      , m_shadowAtlas(pCmdList)
      , m_lightData(BufferUsageHint_Storage | BufferUsageHint_Dynamic)
      , m_shadowMaps(BufferUsageHint_Dynamic)
      , m_depthPass(pAssets, URI::File("engine:shaders/general/depth-pass.shader"))
      , m_shader(pAssets, URI::File("engine:shaders/pbr/lighting.shader")) {}

    virtual void onAdded(graphics::CommandList * pCmdList, Renderer * pRenderer) override {
      // Setup shadow map render target
      GraphicsDevice * pDevice = pRenderer->getGraphicsDevice();
      m_shadowMapTarget        = pCmdList->createRenderTarget(RenderTargetType_Texture);
    }

    virtual void beginView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) override {
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

        m_shadowMaps.upload(pCmdList);
      }
      m_lightData.upload(pCmdList);
    }

    virtual void renderView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) override {
      GraphicsDevice * pDevice = pRenderer->getGraphicsDevice();

      /// Render shadow maps
      if (m_shadowMapData.size() > 0) {
        pCmdList->bindProgram(m_depthPass);

        pCmdList->pushState(graphics::State::EnableDepthRead{true}, graphics::State::EnableDepthWrite{true}, graphics::State::ColourWrite{false});

        for (ShadowMapData const & shadowMapData : m_shadowMapData) {
          if (shadowMapData.meshCasters.size() == 0) {
            continue;
          }

          ShadowAtlas::Slot slot = m_shadowAtlas.getSlot(shadowMapData.atlasIndex);
          m_shadowMapTarget->attachDepth(m_shadowAtlas, slot.level, slot.layer);
          pCmdList->bindRenderTarget(m_shadowMapTarget);
          pCmdList->clear(0);
          pCmdList->pushState(graphics::State::Viewport{{0, 0}, m_shadowAtlas.resolution(shadowMapData.atlasIndex)});

          for (MeshShadowCasterRenderable const & caster : view.pRenderData->renderables<MeshShadowCasterRenderable>()) {
            if (geometry::intersects(shadowMapData.lightFrustum, caster.bounds)) {
              m_pModelData->data.mvpMatrix = (Mat4d)shadowMapData.lightVP * caster.modelMatrix;
              m_pModelData->upload(pCmdList);

              pCmdList->bindVertexArray(caster.vertexArray);
              pCmdList->drawIndexed(caster.elementCount, caster.elementOffset);
            }
          }

          pCmdList->popState();
        }

        pCmdList->popState();
      }

      /// Find final scene colour texture.
      /// This target accumulates post-processing effects
      pCmdList->bindRenderTarget(*m_pColourTarget);
      pCmdList->pushState(graphics::State::Viewport{*m_pColourTarget}, graphics::State::EnableDepthRead{false}, graphics::State::EnableDepthWrite{false},
                          graphics::State::EnableStencilTest{false}, graphics::State::EnableBlend{false});

      // Bind post-process texture inputs.
      // TODO: Generalize this using a PostProcessInputRenderable/GBufferInputRenderable?
      for (auto & [i, tex] : enumerate(*m_pGBuffer)) {
        pCmdList->bindTexture(tex, DeferredRenderer::ColourTargetBindPointBase + i);
      }
      pCmdList->bindTexture(m_shadowAtlas, DeferredRenderer::ColourTargetBindPointBase + GBufferTarget_Count);
      pCmdList->bindShaderStorageBuffer(m_shadowMaps, 3);

      pCmdList->bindProgram(m_shader);
      pCmdList->bindVertexArray(InvalidGraphicsResource);
      pCmdList->bindShaderStorageBuffer(m_lightData, renderer::BufferBinding_LightBuffer);
      pCmdList->draw(3);

      pCmdList->popState();
      pCmdList->bindRenderTarget(view.renderTarget); // TODO: Maybe a "push render target" function?
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
    graphics::RenderTargetRef m_shadowMapTarget;
    ShadowAtlas               m_shadowAtlas;

    Asset<graphics::Program> m_depthPass;

    Vector<ShadowMapData>                                      m_shadowMapData; // Data rendered per shadow map
    graphics::StructuredArrayBuffer<renderer::ShadowMapBuffer> m_shadowMaps;
    graphics::StructuredBuffer<renderer::ModelBuffer> *        m_pModelData = nullptr;

    // Lighting pass
    GBuffer *                   m_pGBuffer = nullptr;
    Asset<graphics::Program>    m_shader;
    graphics::RenderTargetRef * m_pColourTarget = nullptr;

    graphics::StructuredArrayBuffer<renderer::LightBuffer> m_lightData;
  };

  class Feature_Skybox : public FeatureRenderer {
  public:
    Feature_Skybox(AssetManager * pAssets, graphics::RenderTargetRef * pColourTarget)
      : m_shader(pAssets, URI::File("engine:shaders/general/cubemap.shader"))
      , m_pColourTarget(pColourTarget) {}

    virtual void renderView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) override {
      const auto & skyboxes = view.pRenderData->renderables<CubeMapRenderable>();
      if (skyboxes.size() == 0) {
        return;
      }

      /// Bind final scene colour texture.
      /// This target accumulates post-processing effects
      pCmdList->bindRenderTarget(*m_pColourTarget);
      pCmdList->pushState(graphics::State::Viewport{{0, 0}, (*m_pColourTarget)->getSize()}, graphics::State::EnableDepthRead{true},
                          graphics::State::EnableDepthWrite{false}, graphics::State::DepthFunc{ComparisonFunction_Equal},
                          graphics::State::EnableStencilTest{false}, graphics::State::EnableBlend{false}, graphics::State::DepthRange{1, 1});

      pCmdList->bindProgram(m_shader);
      pCmdList->bindVertexArray(InvalidGraphicsResource);
      for (auto const & cm : skyboxes) {
        pCmdList->bindTexture(cm.texture, 0);
      }
      pCmdList->draw(6);
      pCmdList->bindRenderTarget(view.renderTarget);

      pCmdList->popState();
    }

    Asset<graphics::Program>    m_shader;
    graphics::RenderTargetRef * m_pColourTarget = nullptr;
  };

  class Feature_SSAO : public FeatureRenderer {
  public:
    Feature_SSAO(AssetManager * pAssets, PostProcessingStack * pPPS)
      : m_pPPS(pPPS)
      , m_ssaoShader(pAssets, URI::File("engine:shaders/ssao/ssao.shader")) {}

    virtual void onAdded(graphics::CommandList * pCmdList, Renderer * pRenderer) override {
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
      graphics::loadTexture2D(pCmdList, &m_randomTex, {m_randSize, m_randSize}, randData.data());
    }

    virtual void beginView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) override {
      auto & ssaoOpts = view.pRenderData->renderables<PostProcessRenderable_SSAO>();
      if (ssaoOpts.size() == 0)
        return;

      PostProcessRenderable_SSAO & ssao = ssaoOpts.front();

      float bias     = ssao.bias;
      float radius   = ssao.radius;
      float strength = ssao.strength;

      m_pPPS->addPass([=](graphics::CommandList * pCmdList, PostProcessParams const & params) {
        params.bindInputs(pCmdList);
        params.bindTarget(pCmdList);
        pCmdList->bindProgram(m_ssaoShader);
        pCmdList->bindTexture(m_randomTex, 7);
        pCmdList->setUniform("radius", radius);
        pCmdList->setUniform("strength", strength);
        pCmdList->setUniform("bias", bias);
        pCmdList->setUniform("outputSize", params.sceneColour->getSize());

        for (int64_t i = 0; i < m_sampleKernel.size(); ++i) {
          pCmdList->setUniform(String::format("sampleKernel[%lld]", i), m_sampleKernel[i]);
        }

        pCmdList->bindVertexArray(InvalidGraphicsResource);
        pCmdList->draw(3);
      });
    }

    Asset<graphics::Program> m_ssaoShader;
    graphics::TextureRef     m_randomTex;
    Vector<Vec3>             m_sampleKernel;
    int64_t                  m_kernelSize = 32;
    int64_t                  m_randSize   = 4;
    PostProcessingStack *    m_pPPS       = nullptr;
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

    virtual void onAdded(graphics::CommandList * pCmdList, Renderer * pRenderer) override {
      m_clampSampler     = pCmdList->createSampler();
      m_mipChainTarget   = pCmdList->createRenderTarget(RenderTargetType_Texture);
      m_reflectionTarget = pCmdList->createRenderTarget(RenderTargetType_Texture);

      m_clampSampler->setSamplerWrap(WrapMode_ClampToEdge);
    }

    virtual void beginView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) override {
      auto & ssrOpt = view.pRenderData->renderables<PostProcessRenderable_SSR>();
      if (ssrOpt.size() == 0)
        return;

      Vec2i size = view.renderTarget->getSize();
      if (size != m_chainSize) {
        graphics::loadTexture2D(pCmdList, &m_reflectionUV, size, PixelFormat_RGBAf16);
        m_reflectionTarget->attachColour(m_reflectionUV);

        m_mipChain.resize(m_mipChainSize);
        m_chainSize     = size;
        Vec2i levelSize = size;
        for (int64_t i = 0; i < m_mipChainSize; ++i) {
          graphics::loadTexture2D(pCmdList, &m_mipChain[i], levelSize, PixelFormat_RGBAf16);
          levelSize /= 2;
        }
      }

      auto & opt         = ssrOpt.front();
      float  maxDistance = opt.maxDistance;
      float  resolution  = opt.resolution;
      int    steps       = opt.steps;
      float  thickness   = opt.thickness;

      m_pPPS->addPass([=](graphics::CommandList * pCmdList, PostProcessParams const & params) {
        Vec2i targetSize = params.target->getSize();

        // Calculate reflections
        params.bindInputs(pCmdList);
        pCmdList->bindRenderTarget(m_reflectionTarget);
        pCmdList->pushState(graphics::State::Viewport{m_mipChain[0]});
        pCmdList->bindProgram(programs.calculateReflections);
        pCmdList->setUniform("maxDistance", maxDistance);
        pCmdList->setUniform("resolution", resolution);
        pCmdList->setUniform("steps", steps);
        pCmdList->setUniform("thickness", thickness);
        pCmdList->setUniform("outputSize", targetSize);

        params.bindInputs(pCmdList);
        pCmdList->bindVertexArray(InvalidGraphicsResource);
        pCmdList->draw(3);
        // Blur reflections (for roughness)
        pCmdList->bindProgram(programs.downsampler);
        pCmdList->bindVertexArray(InvalidGraphicsResource);
        pCmdList->bindSampler(m_clampSampler, 0);
        pCmdList->bindTexture(params.sceneColour, 0);
        for (int64_t i = 1; i < m_mipChain.size(); ++i) {
          m_mipChainTarget->attachColour(m_mipChain[i]);
          pCmdList->bindRenderTarget(m_mipChainTarget);
          pCmdList->setState(graphics::State::Viewport{m_mipChain[i]});
          pCmdList->draw(3);
          pCmdList->bindTexture(m_mipChain[i], 0);
        }

        // Blend reflections with final output
        params.bindInputs(pCmdList);
        params.bindTarget(pCmdList);
        pCmdList->bindProgram(programs.blendReflections);
        pCmdList->bindTexture(m_reflectionUV, 7);
        pCmdList->bindTexture(m_mipChain.back(), 8);
        pCmdList->bindTexture(m_brdfLUT, 9);
        pCmdList->draw(3);
        pCmdList->popState();
      });
    }

  private:
    graphics::SamplerRef m_clampSampler;

    struct {
      Asset<graphics::Program> calculateReflections;
      Asset<graphics::Program> blendReflections;
      Asset<graphics::Program> upsampler;
      Asset<graphics::Program> downsampler;
    } programs;

    int64_t m_mipChainSize = 7;

    graphics::TextureRef m_brdfLUT; // TODO: Calculate brdf LUT

    graphics::TextureRef m_reflectionUV;
    graphics::RenderTargetRef m_reflectionTarget;

    graphics::RenderTargetRef m_mipChainTarget;
    Vec2i                   m_chainSize;
    Vector<graphics::TextureRef> m_mipChain;

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

    virtual void onAdded(graphics::CommandList * pCmdList, Renderer * pRenderer) override {
      m_mipChainTarget                    = pCmdList->createRenderTarget(RenderTargetType_Texture);
      m_filteredTarget = pCmdList->createRenderTarget(RenderTargetType_Texture);
      m_clampSampler                      = pCmdList->createSampler();
      m_clampSampler->setSamplerWrap(WrapMode_ClampToEdge);
    }

    virtual void onResize(graphics::CommandList * pCmdList, Renderer * pRenderer, Vec2i const & size) override {}

    virtual void beginFrame(graphics::CommandList * pCmdList, Renderer * pRenderer, Vector<RenderView> const & views) {}

    virtual void beginView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) override {
      Vec2i size = view.renderTarget->getSize();
      if (size != m_chainSize) {
        m_mipChain.resize(m_mipChainSize);
        m_chainSize     = size;
        Vec2i levelSize = size;
        for (int64_t i = 0; i < m_mipChainSize; ++i) {
          levelSize /= 2;
          graphics::loadTexture2D(pCmdList, &m_mipChain[i], levelSize, PixelFormat_RGBAf16);
        }
        graphics::loadTexture2D(pCmdList, &m_filtered, size, PixelFormat_RGBAf16);
        m_filteredTarget->attachColour(m_filtered);
      }

      auto & bloomOpts = view.pRenderData->renderables<PostProcessRenderable_Bloom>();
      if (bloomOpts.size() == 0)
        return;
      float            filterRadius  = bloomOpts.front().filterRadius;
      float            strength      = bloomOpts.front().strength;
      float            threshold     = bloomOpts.front().threshold;
      float            dirtIntensity = bloomOpts.front().dirtIntensity;
      graphics::TextureRef dirtTex       = bloomOpts.front().dirtTex;

      m_pPPS->addPass([=](graphics::CommandList * pCmdList, PostProcessParams const & params) {
        if (threshold > 0) {
          pCmdList->bindProgram(m_prefilter);
          pCmdList->setUniform("threshold", threshold);
          pCmdList->bindRenderTarget(m_filteredTarget);
          pCmdList->pushState(graphics::State::Viewport{{0, 0}, m_filteredTarget->getSize()});
          pCmdList->bindTexture(params.sceneColour, 0);
          pCmdList->draw(3);
          pCmdList->popState();
          pCmdList->bindTexture(m_filtered, 0);
        } else {
          pCmdList->bindTexture(params.sceneColour, 0);
        }

        // Down-sample the hdr colour texture
        pCmdList->bindProgram(m_downsampler);
        pCmdList->bindVertexArray(InvalidGraphicsResource);
        pCmdList->bindSampler(m_clampSampler, 0);
        for (int64_t i = 0; i < m_mipChain.size(); ++i) {
          m_mipChainTarget->attachColour(m_mipChain[i]);
          pCmdList->bindRenderTarget(m_mipChainTarget);
          pCmdList->pushState(graphics::State::Viewport{{0, 0}, m_mipChain[i]->getSize()});
          pCmdList->draw(3);
          pCmdList->popState();
          pCmdList->bindTexture(m_mipChain[i], 0);
        }

        // Up-sample down-sampled colour texture
        pCmdList->bindProgram(m_upsampler);
        pCmdList->setUniform("filterRadius", filterRadius);

        pCmdList->pushState(graphics::State::EnableBlend{true}, graphics::State::BlendEq{BlendEquation_Add},
                            graphics::State::BlendFunc{BlendFunction_One, BlendFunction_One});

        pCmdList->bindVertexArray(InvalidGraphicsResource);
        for (int64_t i = m_mipChain.size() - 1; i > 0; --i) {
          m_mipChainTarget->attachColour(m_mipChain[i - 1]);
          pCmdList->bindRenderTarget(m_mipChainTarget);
          pCmdList->pushState(graphics::State::Viewport{{0, 0}, m_mipChain[i - 1]->getSize()});
          pCmdList->bindTexture(m_mipChain[i], 0);
          pCmdList->draw(3);

          pCmdList->popState();
        }
        pCmdList->popState();

        pCmdList->pushState(graphics::State::EnableBlend{false}, graphics::State::BlendEq{BlendEquation_Add},
                            graphics::State::BlendFunc{BlendFunction_SourceAlpha, BlendFunction_OneMinusSourceAlpha});

        // Bind final render target
        params.bindTarget(pCmdList);
        params.bindInputs(pCmdList);

        // Mix bloom with source colour
        pCmdList->bindProgram(m_blendBloom);
        pCmdList->setUniform("strength", strength);
        pCmdList->setUniform("dirtIntensity", dirtIntensity);
        pCmdList->bindTexture(params.sceneColour, 0);
        pCmdList->bindTexture(m_mipChain.front(), 1);
        pCmdList->bindTexture(dirtTex, 2);
        pCmdList->draw(3);
        pCmdList->bindSampler(InvalidGraphicsResource, 0);

        pCmdList->popState();
      });
    }

    PostProcessingStack * m_pPPS = nullptr;

    Asset<graphics::Program> m_upsampler;
    Asset<graphics::Program> m_downsampler;
    Asset<graphics::Program> m_blendBloom;
    Asset<graphics::Program> m_prefilter;

    graphics::SamplerRef m_clampSampler;

    int64_t m_mipChainSize = 5;

    graphics::RenderTargetRef m_mipChainTarget;
    Vec2i                   m_chainSize;
    Vector<graphics::TextureRef> m_mipChain;

    graphics::RenderTargetRef m_filteredTarget;
    graphics::TextureRef      m_filtered;
  };

  class Feature_Exposure : public FeatureRenderer {
  public:
    Feature_Exposure(AssetManager * pAssets, PostProcessingStack * pPPS)
      : m_pPPS(pPPS)
      , m_shader(pAssets, URI::File("engine:shaders/simple-tonemapper.shader")) {}

    virtual void beginView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) override {
      GraphicsDevice * pDevice = pRenderer->getGraphicsDevice();

      auto  exposureRenderables = view.pRenderData->renderables<PostProcessRenderable_Exposure>();
      float exposure            = 1.0f;
      if (exposureRenderables.size() > 0)
        exposure = exposureRenderables.front().exposure;

      m_pPPS->addPass([=](graphics::CommandList * pCmdList, PostProcessParams const & params) {
        // Currently only support tone-mapping post process effect.
        // TODO: Add support for more effects (maybe custom effects as well?)
        params.bindInputs(pCmdList);
        params.bindTarget(pCmdList);

        pCmdList->bindProgram(m_shader);
        pCmdList->setUniform("exposure", exposure);
        pCmdList->bindVertexArray(InvalidGraphicsResource);
        pCmdList->draw(3);
      });
    }

    Asset<graphics::Program> m_shader;
    PostProcessingStack *    m_pPPS = nullptr;
  };

  class Feature_PostProcess : public FeatureRenderer {
  public:
    Feature_PostProcess(AssetManager * pAssets, PostProcessingStack * pPPS, GBuffer * pGBuffer, graphics::TextureRef * pFinalColour)
      : m_pPPS(pPPS)
      , m_pGBuffer(pGBuffer)
      , m_pFinalColour(pFinalColour) {}

    virtual void renderView(graphics::CommandList * pCmdList, Renderer * pRenderer, RenderView const & view) override {
      m_pPPS->target               = view.renderTarget;
      m_pPPS->sceneColour          = *m_pFinalColour;
      m_pPPS->inputs.sceneDepth    = m_pGBuffer->getDepthTarget();
      m_pPPS->inputs.baseColour    = m_pGBuffer->getBaseColour();
      m_pPPS->inputs.ambientColour = m_pGBuffer->getAmbientColour();
      m_pPPS->inputs.normal        = m_pGBuffer->getNormal();
      m_pPPS->inputs.position      = m_pGBuffer->getPosition();
      m_pPPS->inputs.rma           = m_pGBuffer->getRMA();

      m_pPPS->execute(pCmdList, view.renderTarget->getSize());
    }

    PostProcessingStack *  m_pPPS         = nullptr;
    GBuffer *              m_pGBuffer     = nullptr;
    graphics::TextureRef * m_pFinalColour = nullptr;
  };

  DeferredRenderer::DeferredRenderer(graphics::CommandList * pCmdList, AssetManager * pAssets)
    : Renderer(pCmdList->getDevice())
    , m_gbuffer(pCmdList, {1920, 1080})
    , m_modelData(BufferUsageHint_Uniform | BufferUsageHint_Dynamic)
    , m_cameraData(BufferUsageHint_Uniform | BufferUsageHint_Dynamic) {
    // Create renderer resources
    graphics::loadTexture2D(pCmdList, &m_finalColourTarget, {1920, 1080}, PixelFormat_RGBAf16);

    m_finalTarget = pCmdList->createRenderTarget(RenderTargetType_Texture);
    m_finalTarget->attachColour(m_finalColourTarget);
    m_finalTarget->attachDepth(m_gbuffer.getDepthTarget(), 0, 0);

    Colour<RGBAu8> white[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    Colour<RGBAu8> black[4] = {0, 0, 0, 0};
    Colour<RGBAu8> blue[4]  = {{127, 127, 255, 1}, {127, 127, 255, 1}, {127, 127, 255, 1}, {127, 127, 255, 1}};

    // Load some default textures
    graphics::TextureRef whiteTex, blackTex, blueTex;
    graphics::loadTexture2D(pCmdList, &whiteTex, {2, 2}, white);
    graphics::loadTexture2D(pCmdList, &blackTex, {2, 2}, black);
    graphics::loadTexture2D(pCmdList, &blueTex, {2, 2}, blue);

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

    m_defaultMaterial.upload(pCmdList);

    // Add renderer features
    addFeature<Feature_MeshBasePass>(pAssets, &m_gbuffer, &m_modelData);
    addFeature<Feature_LightingPass>(pCmdList, pAssets, &m_gbuffer, &m_finalTarget, &m_modelData);
    addFeature<Feature_Skybox>(pAssets, &m_finalTarget);
    addFeature<Feature_SSAO>(pAssets, &m_postProc);
    addFeature<Feature_SSR>(pAssets, &m_postProc);
    addFeature<Feature_Bloom>(pAssets, &m_postProc);
    addFeature<Feature_Exposure>(pAssets, &m_postProc);
    addFeature<Feature_PostProcess>(pAssets, &m_postProc, &m_gbuffer, &m_finalColourTarget);
  }

  graphics::RenderTargetRef DeferredRenderer::getFinalTarget() const {
    return m_finalTarget;
  }

  graphics::TextureRef const & DeferredRenderer::getDefaultTexture(Material::TextureSlot slot) const {
    return m_defaultMaterialTextures[slot];
  }

  graphics::StructuredBuffer<renderer::PBRMaterial> const & DeferredRenderer::getDefaultMaterial() const {
    return m_defaultMaterial;
  }

  void DeferredRenderer::onResize(graphics::CommandList * pCmdList, Vec2i size) {
    Renderer::onResize(pCmdList, size);

    m_gbuffer.resize(pCmdList, size);
    graphics::loadTexture2D(pCmdList, &m_finalColourTarget, size, PixelFormat_RGBAf16);
  }

  void DeferredRenderer::render(graphics::CommandList * pCmdList, Vector<RenderView> const & views) {
    pCmdList->bindUniformBuffer(m_cameraData, renderer::BufferBinding_CameraBuffer);
    pCmdList->bindUniformBuffer(m_modelData, renderer::BufferBinding_ModelBuffer);
    m_postProc.reset();

    Renderer::render(pCmdList, views);
  }

  void DeferredRenderer::beginView(graphics::CommandList * pCmdList, RenderView const & view) {
    Mat4d dVP = view.projectionMatrix * view.viewMatrix;

    m_cameraData.data.viewProjMatrix    = dVP;
    m_cameraData.data.invViewProjMatrix = glm::inverse(dVP);
    m_cameraData.data.projMatrix        = view.projectionMatrix;
    m_cameraData.data.viewMatrix        = view.viewMatrix;
    m_cameraData.data.invProjMatrix     = glm::inverse(view.projectionMatrix);
    m_cameraData.data.invViewMatrix     = glm::inverse(view.viewMatrix);
    m_cameraData.upload(pCmdList);
  }

  void DeferredRenderer::endView(graphics::CommandList * pCmdList, RenderView const & view) {
    BFC_UNUSED(pCmdList, view);
  }
} // namespace engine
