#version 430

#include "../common.glsl"
#include "../postprocessinput.glsl"
#include "../lighting-pbr.glsl"

in vec2 vsout_uv0;
in vec3 vsout_position0;

layout (binding=7) uniform sampler2D reflectionUV;
layout (binding=8) uniform sampler2D blurredSceneColourTex;

out vec4 fragColour;

void main() {
  vec4 scene    = pps_ReadSceneColour(vsout_uv0);
  vec3 base     = pps_ReadBaseColour(vsout_uv0);
  vec3 position = pps_ReadPosition(vsout_uv0);
  vec3 rma      = pps_ReadRMA(vsout_uv0);
  vec3 normal   = pps_ReadNormal(vsout_uv0);
  
  float roughness = rma.x;
  float metalness = rma.y;
  float ao        = rma.z;
  
  vec3 camPos = invViewMatrix[3].xyz;

  vec3 N = normal;
  vec3 V = normalize(camPos - position);
  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, base, metalness);

  vec3 F   = FresnelSchlick(max(dot(N, V), 0.0), F0);
  // vec2 brdf  = texture(BRDFLut, vec2(max(dot(N, V), 0.0), roughness)).rg;
  vec2 brdf = vec2(1, 0);

  // Sample reflections
  vec4 uv      = textureLod(reflectionUV, vsout_uv0, 0);
  vec4 sharp   = textureLod(sceneColourTex, uv.xy, 0);
  vec4 blurred = textureLod(blurredSceneColourTex, uv.xy, 0);
  vec3 reflection = mix(sharp.xyz, blurred.xyz, roughness) * (F * brdf.x + brdf.y);

  fragColour = vec4(scene.xyz + reflection * uv.a, scene.a);
}
