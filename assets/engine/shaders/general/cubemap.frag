#version 430

// TODO: Handle Includes
// #include "../common.h.glsl"

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

// -----------------------------

layout(binding = 0) uniform samplerCube texture0;

in vec3 vsout_direction0;

out vec4 psout_colour0;

void main()
{
  psout_colour0 = vec4(texture(texture0, vsout_direction0).rgb, 1);
}
