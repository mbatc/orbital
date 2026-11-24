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
const float FLT_MAX = 3.402823466e+38;

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

vec3 rotateAxis(vec3 p, vec3 axis, float angle) {
  return mix(dot(axis, p) * axis, p, cos(angle)) + cross(axis, p) * sin(angle);
}

vec3 getModelPosition() {
  return modelMatrix[3].xyz;
}

vec3 getCameraPosition() {
  return invViewMatrix[3].xyz;
}

vec3 getCameraForward() {
  return invViewMatrix[2].xyz;
}

vec2 getScreenUV(vec3 worldPosition) {
  vec4 position = (viewProjMatrix * vec4(worldPosition, 1));
  position /= position.w;
  return (position.xy + vec2(1)) / 2;
}

vec2 rotate2d(vec2 v, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    mat2 m = mat2(c, s, -s, c);
    return m * v;
}

vec3 rotate3d(vec3 v, vec3 axis, float angle) {
  float c = cos(angle);
  float s = sin(angle);
  return v * c + cross(axis, v) * s + axis * dot(axis, v) * (1.0 - c);
}

vec3 projectPointOnPlane(vec3 point, vec3 planePos, vec3 planeNormal) {
    float dist = dot(point - planePos, planeNormal);
    return point - planeNormal * dist;
}

vec3 projectVectorOnPlane(vec3 v, vec3 planeNormal) {
    return v - planeNormal * dot(v, planeNormal);
}

#endif // COMMON_GLSL
