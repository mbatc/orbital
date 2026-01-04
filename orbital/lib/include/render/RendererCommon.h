#pragma once

#include "math/MathTypes.h"

namespace bfc {
  enum BlendMode {
    BlendMode_Opaque,
    BlendMode_Add,
    BlendMode_Subtract,
    BlendMode_Multiply,
    BlendMode_Count,
  };

  namespace renderer {
    struct CameraBuffer {
      Mat4 viewProjMatrix;
      Mat4 viewMatrix;
      Mat4 projMatrix;
      Mat4 invViewProjMatrix;
      Mat4 invViewMatrix;
      Mat4 invProjMatrix;

      // Helper for setting all the matrices in the buffer
      inline void setMatrices(Mat4 view, Mat4 projection) {
        viewMatrix     = view;
        projMatrix     = projection;
        viewProjMatrix = projection * view;
        invViewMatrix     = glm::inverse(viewMatrix);
        invProjMatrix     = glm::inverse(projMatrix);
        invViewProjMatrix = glm::inverse(viewProjMatrix);
      }
    };

    struct ModelBuffer {
      Mat4 modelMatrix;
      Mat4 normalMatrix;
      Mat4 mvpMatrix;
    };

    struct LightBuffer {
      Vec3 position;
      int32_t type;

      Vec3 colour;
      float padding0;

      Vec3 ambient;
      float strength;

      Vec3 attenuation;
      float innerCutoff;

      Vec3 direction;
      float outerCutoff;

      Vec2u shadowMapRange;
      float padding1[2];
    };

    struct ShadowMapBuffer {
      Mat4  lightVP; ///< Light view projection matrix (shadow map rendered with this).
      int32_t layer; ///< Shadow map slot array index
      int32_t level; ///< Shadow map slot mip-level

      float padding0[2];
    };

    struct PBRMaterial {
      Vec4 albedo = Vec4(1);
      Vec4 ambient = Vec4(0);
      Vec4 emissive = Vec4(0);

      float roughness = 1;
      float metalness = 0;
      float alpha = 1;
      float padding[1] = { 0 };
    };

    enum BufferBinding {
      BufferBinding_CameraBuffer,
      BufferBinding_ModelBuffer,
      BufferBinding_LightBuffer,
      BufferBinding_PBRMaterial,
      BufferBinding_Count,
    };

    inline bfc::Mat4d calcNormalMatrix(bfc::Mat4d const & modelMatrix) {
      return glm::transpose(glm::inverse(modelMatrix));
    }

    inline bfc::Mat4d calcMvpMatrix(bfc::Mat4d const & modelMatrix, bfc::Mat4d const & viewMatrix, bfc::Mat4d const & projectionMatrix) {
      return projectionMatrix * viewMatrix * modelMatrix;
    }

    inline bfc::Mat4d calcMvpMatrix(bfc::Mat4d const & modelMatrix, bfc::Mat4d const & viewProjectionMatrix) {
      return viewProjectionMatrix * modelMatrix;
    }
  }
}

