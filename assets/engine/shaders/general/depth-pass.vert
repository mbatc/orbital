#version 430

// TODO: Handle Includes
// #include "common.h.glsl"

// common.h.glsl
// -----------------------------

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
  float outerCutoff;
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

// -----------------------------

in layout(location = 0) vec3 position0;
in layout(location = 2) vec2 uv0;
in layout(location = 3) vec3 normal0;
in layout(location = 4) vec3 tangent0;

void main()
{
  gl_Position = mvpMatrix * vec4(position0, 1);
}
