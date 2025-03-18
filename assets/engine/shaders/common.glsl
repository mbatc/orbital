#ifndef COMMON_GLSL
#define COMMON_GLSL

#define LOC_POSITION0 0
#define LOC_COLOUR0   1
#define LOC_UV0       2
#define LOC_NORMAL0   3
#define LOC_TANGENT0  4

const float PI = 3.14159265359;

#define LightType_Directional 0
#define LightType_Point 0
#define LightType_Spot 0

struct LightBuffer {
  vec3 position;
  int  type;

  vec3 colour;
  float padding0;

  vec3 ambient;
  float strength;

  vec3  attenuation;
  float innerCutoff;

  vec3  direction;
  float coneCutoff;

  ivec2 shadowMapRange;
  float padding1;
};

layout(std140, binding=0) uniform Camera {
  mat4 viewProjMatrix;
  mat4 viewMatrix;
  mat4 projMatrix;
  mat4 invViewProjMatrix;
  mat4 invViewMatrix;
  mat4 invProjMatrix;
};

layout(std140, binding=1) uniform Model {
  mat4 modelMatrix;
  mat4 normalMatrix;
  mat4 mvpMatrix;
};

layout(std140, binding = 3) buffer lights {
  LightBuffer lightData[];
};

vec3 SampleCube(const vec3 dir)
{
  vec3 absDir = abs(dir);
  vec2 uv = vec2(0);
  float face = 0;
  float ma = 0;

  if (absDir.z >= absDir.x && absDir.z >= absDir.y) {
    face = dir.z < 0.0 ? 5.0 : 4.0;
    ma = 0.5 / absDir.z;
    uv = vec2(dir.z < 0.0 ? -dir.x : dir.x, -dir.y);
  } else if (absDir.y >= absDir.x) {
    face = dir.y < 0.0 ? 3.0 : 2.0;
    ma = 0.5 / absDir.y;
    uv = vec2(dir.x, dir.y < 0.0 ? -dir.z : dir.z);
  } else {
    face = dir.x < 0.0 ? 1.0 : 0.0;
    ma = 0.5 / absDir.x;
    uv = vec2(dir.x < 0.0 ? dir.z : -dir.z, -dir.y);
  }

  return vec3(uv * ma + 0.5, face);
}

#endif // COMMON_GLSL
