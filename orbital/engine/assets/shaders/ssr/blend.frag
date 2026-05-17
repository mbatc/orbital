#version 430

#include "../common.h.glsl"
#include "../postprocessinput.h.glsl"
#include "../lighting-pbr.h.glsl"

in vec2 vsout_uv0;
in vec3 vsout_position0;

out vec4 fragColour;

void main() {
  // Sample GBuffers
  // TODO: Add sample functions to gbuffer/postprocessinput include
  vec4 scene    = texture2D(sceneColourTex, vsout_uv0);
  vec3 base     = texture2D(baseColourTex, vsout_uv0).xyz;
  vec3 position = texture2D(positionTex, vsout_uv0).xyz;
  vec3 rma      = texture2D(RMATex, vsout_uv0).xyz;
  vec3 normal   = normalize(2 * (texture2D(normalTex, vsout_uv0).xyz - vec3(0.5)));
  
  float roughness = rma.x;
  float metalness = rma.y;
  float ao        = rma.z;
  
  vec3 camPos = invViewMatrix[3].xyz;

  vec3 N = normal;
  vec3 V = normalize(camPos - position);
  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, base, metalness);

  vec3 F   = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
  // vec2 brdf  = texture(BRDFLut, vec2(max(dot(N, V), 0.0), roughness)).rg;
  vec2 brdf = vec2(1, 0);

  // Sample reflections
  vec4 uv      = textureLod(reflectionUV, vsout_uv0, 0);
  vec4 sharp   = textureLod(sceneColourTex, uv.xy, 0);
  vec4 blurred = textureLod(blurredSceneColourTex, uv.xy, 0);
  vec3 reflection = mix(sharp.xyz, blurred.xyz, roughness) * (F * brdf.x + brdf.y);

  fragColour = vec4(scene.xyz + reflection * uv.a, scene.a);
}
