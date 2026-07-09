#version 430

#include "../gbuffer-out.glsl"
#include "../gbuffer.glsl"

in vec2 vsout_uv0;

void main() {
  vec3 baseColour    = textureLod(G_BaseColour, vsout_uv0, 0).rgb;
  vec3 transmittance = textureLod(G_Ambient, vsout_uv0, 0).rgb;

  vec3 outColour = mix(baseColour, transmittance, length(transmittance));

  gbuffer_output[GBUFFER_COLOUR] = vec4(outColour, 1); // textureLod(G_BaseColour, vsout_uv0, 0);
  gbuffer_output[GBUFFER_POSITION] = textureLod(G_Position, vsout_uv0, 0);
  gbuffer_output[GBUFFER_AMBIENT] = vec4(0); // textureLod(G_Ambient, vsout_uv0, 0);
  gbuffer_output[GBUFFER_NORMAL] = textureLod(G_Normal, vsout_uv0, 0);
  gbuffer_output[GBUFFER_RMAO] = textureLod(G_RMA, vsout_uv0, 0);
}
