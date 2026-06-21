#version 430

#include "../gbuffer-out.glsl"
#include "../gbuffer.glsl"

in vec2 vsout_uv0;

void main() {
  gbuffer_output[GBUFFER_COLOUR] = textureLod(G_BaseColour, vsout_uv0, 0);
  gbuffer_output[GBUFFER_POSITION] = textureLod(G_Position, vsout_uv0, 0);
  gbuffer_output[GBUFFER_AMBIENT] = textureLod(G_Ambient, vsout_uv0, 0);
  gbuffer_output[GBUFFER_NORMAL] = textureLod(G_Normal, vsout_uv0, 0);
  gbuffer_output[GBUFFER_RMAO] = textureLod(G_RMA, vsout_uv0, 0);
}
