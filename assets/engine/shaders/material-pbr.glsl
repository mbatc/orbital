#ifndef MATERIAL_PBR_GLSL
#define MATERIAL_PBR_GLSL

layout(std140, binding=3) uniform Material {
  vec4 albedo;
  vec4 ambient;
  vec4 emissive;
  float roughness;
  float metalness;
  float padding[2];
};

layout(binding = 0) uniform sampler2D baseColourMap;
layout(binding = 1) uniform sampler2D ambientMap;
layout(binding = 2) uniform sampler2D emissiveMap;
layout(binding = 3) uniform sampler2D roughnessMap;
layout(binding = 4) uniform sampler2D metalnessMap;
layout(binding = 5) uniform sampler2D aoMap;
layout(binding = 6) uniform sampler2D alphaMap;
layout(binding = 7) uniform sampler2D normalMap;

#endif // MATERIAL_PBR_GLSL