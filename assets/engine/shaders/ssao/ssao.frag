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

layout(binding = 0) uniform sampler2D sceneColourTex;
layout(binding = 1) uniform sampler2D sceneDepthTex;
layout(binding = 2) uniform sampler2D baseColourTex;
layout(binding = 3) uniform sampler2D ambientTex;
layout(binding = 4) uniform sampler2D positionTex;
layout(binding = 5) uniform sampler2D normalTex;
layout(binding = 6) uniform sampler2D RMATex;

// ----------------------------------

layout(binding = 7) uniform sampler2D randomTex;

#define KERNEL_SIZE 32

uniform float bias;
uniform float radius;
uniform float strength;

uniform vec3 sampleKernel[KERNEL_SIZE];
uniform ivec2 outputSize;

in vec2 vsout_uv0;
in vec3 vsout_position0;

out vec4 fragColour;

void main()
{
  vec3 normal = normalize(2 * (texture(normalTex, vsout_uv0).xyz - vec3(0.5)));
  vec3 position = texture(positionTex, vsout_uv0).rgb;
  vec3 vsNormal = normalize((transpose(inverse(viewMatrix)) * vec4(normal, 1)).xyz);
  vec3 vsPosition = (viewMatrix * vec4(position, 1)).xyz;

  vec2 randSize = textureSize(randomTex, 0);
  vec2 randScale = outputSize / randSize;

  vec3 tangent = (textureLod(randomTex, vsout_uv0 * randScale, 0).xyz - vec3(0.5)) * 2;
  tangent = normalize(tangent - vsNormal * dot(tangent, vsNormal));
  vec3 bitangent = cross(vsNormal, tangent);
  mat3 kernelRot = mat3(tangent, bitangent, vsNormal);

  float occlusion = 0.0f;

  for (int i = 0; i < KERNEL_SIZE; ++i) {
    vec3 sampleNormal = kernelRot * sampleKernel[i];
    if (dot(sampleNormal, vsNormal) < 0) {
      sampleNormal = reflect(sampleNormal, vsNormal);
    }

    vec3 samplePosition = vsPosition + sampleNormal * radius;
    vec4 projectedSample = projMatrix * vec4(samplePosition, 1);
    projectedSample /= projectedSample.w;
    projectedSample.xyz  = projectedSample.xyz * 0.5 + 0.5;

    if (projectedSample.x >= 0 && projectedSample.x <= 1 && projectedSample.y >= 0 && projectedSample.y <= 1)
    {
      float sampleDepth = (viewMatrix * vec4(texture(positionTex, projectedSample.xy).xyz, 1)).z;
      float rangeCheck = smoothstep(0.0, 1.0, radius / abs(vsPosition.z - sampleDepth));
      occlusion       += (sampleDepth >= samplePosition.z + bias ? 1.0 : 0.0) * rangeCheck; 
    }
  }

  occlusion /= KERNEL_SIZE;
  occlusion *= strength;
  fragColour = vec4(texture(sceneColourTex, vsout_uv0).rgb * (1 - occlusion), 1);
}
