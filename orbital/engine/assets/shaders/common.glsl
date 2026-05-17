#ifndef COMMON_GLSL
#define COMMON_GLSL

#define LOC_POSITION0 0
#define LOC_COLOUR0   1
#define LOC_UV0       2
#define LOC_NORMAL0   3
#define LOC_TANGENT0  4

// TODO: These could be defined by the application compiling
//       the shader.
#define BND_UBO_Camera  0
#define BND_UBO_Model   1
#define BND_UBO_Lights  2
#define BND_UBO_Shadows 3

const float PI = 3.14159265359;

layout(std140, binding=BND_UBO_Camera) uniform Camera {
  mat4 viewProjMatrix;
  mat4 viewMatrix;
  mat4 projMatrix;
  mat4 invViewProjMatrix;
  mat4 invViewMatrix;
  mat4 invProjMatrix;
};

layout(std140, binding=BND_UBO_Model) uniform Model {
  mat4 modelMatrix;
  mat4 normalMatrix;
  mat4 mvpMatrix;
};

#endif // COMMON_GLSL
