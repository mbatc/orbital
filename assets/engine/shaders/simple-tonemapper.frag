#version 430

// #include "postprocessinput.h.glsl"

// TODO: Handle Includes
// #include "../common.h.glsl"
// #include "../postprocessinput.h.glsl"

// common.h.glsl
// -----------------------------

#define LightType_Sun   0
#define LightType_Point 1
#define LightType_Spot  2

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

layout(std140, binding=2) buffer lights {
  LightBuffer lightData[];
};

// postprocessinput.h.glsl
// -----------------------------

layout(binding = 0) uniform sampler2D sceneColour;
layout(binding = 1) uniform sampler2D sceneDepth;
layout(binding = 2) uniform sampler2D baseColour;
layout(binding = 3) uniform sampler2D ambient;
layout(binding = 4) uniform sampler2D position;
layout(binding = 5) uniform sampler2D normal;
layout(binding = 6) uniform sampler2D RMA;

// ----------------------------------

out vec4 fragColour;

uniform float exposure;

in vec2 vsout_uv0;
in vec3 vsout_position0;

void main()
{
  const float gamma = 2.2;
  vec3 hdrColor = texture(sceneColour, vsout_uv0).rgb;
  // exposure tone mapping
  vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

  // gamma correction 
  mapped = pow(mapped, vec3(1.0 / gamma));

  fragColour = vec4(mapped, 1.0);
}
