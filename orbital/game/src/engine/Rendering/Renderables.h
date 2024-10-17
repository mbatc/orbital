#pragma once

#include "../Levels/CoreComponents.h"
#include "geometry/Geometry.h"
#include "mesh/Mesh.h"
#include "render/GraphicsDevice.h"

namespace engine {
  /// Mesh render data.
  /// Stores geometry and material information.
  struct MeshRenderable {
    int64_t    elementOffset;
    int64_t    elementCount;
    bfc::Mat4d modelMatrix;
    bfc::Mat4d normalMatrix;

    bfc::GraphicsResource vertexArray;
    bfc::GraphicsResource materialBuffer;
    bfc::GraphicsResource materialTextures[bfc::Material::TextureSlot_Count];

    bfc::geometry::Box<float> bounds;
  };

  /// Exposure fore tone-mapping post process
  struct MeshShadowCasterRenderable {
    int64_t    elementOffset;
    int64_t    elementCount;
    bfc::Mat4d modelMatrix;
    bfc::Mat4d normalMatrix;

    bfc::GraphicsResource vertexArray;

    bfc::geometry::Box<float> bounds;
  };

  /// Skybox render data.
  struct CubeMapRenderable {
    bfc::GraphicsResource texture;
    float                 alpha;
  };

  /// Image-based lighting data.
  struct CubeMapIBLRenderable {
    bfc::GraphicsResource irradiance;
    bfc::GraphicsResource prefilter;
    bfc::GraphicsResource brdfLUT;
    float                 intensity;
  };

  /// Light render data.
  struct LightRenderable {
    components::LightType type;

    bfc::Vec3 position;
    bfc::Vec3 direction;
    bfc::Vec3 colour;
    bfc::Vec3 ambient;
    bfc::Vec3 attenuation;
    float     strength;
    float     innerConeAngle;
    float     outerConeAngle;

    bool castShadows;

    bfc::geometry::Box<float> area; ///< Box encompassing the area this light influences
  };

  /// Settings for tone-mapping post process
  struct PostProcessRenderable_Exposure {
    float exposure;
  };

  /// Settings for bloom post process
  struct PostProcessRenderable_Bloom {
    float strength;
    float filterRadius;
    float threshold;

    bfc::GraphicsResource dirtTex;
    float                 dirtIntensity;
  };

  /// Settings for screen space ambient occlusion post process
  struct PostProcessRenderable_SSAO {
    float strength;
    float radius;
    float bias;
  };

  /// Settings for screen space reflection post process
  struct PostProcessRenderable_SSR {
    float maxDistance;
    float resolution;
    int   steps;
    float thickness;
  };
} // namespace engine

namespace bfc {
  inline bfc::geometry::Sphere<float> calcBoundingSphere(engine::MeshShadowCasterRenderable const & o) {
    return calcBoundingSphere(o.bounds);
  }

  inline bfc::geometry::Sphere<float> calcBoundingSphere(engine::MeshRenderable const & o) {
    return calcBoundingSphere(o.bounds);
  }

  inline bfc::geometry::Box<float> calcBoundingBox(engine::MeshShadowCasterRenderable const & o) {
    return o.bounds;
  }

  inline bfc::geometry::Box<float> calcBoundingBox(engine::MeshRenderable const & o) {
    return o.bounds;
  }
} // namespace bfc
