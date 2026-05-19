#version 430

#include "../material-pbr.glsl"
#include "../gbuffer-out.glsl"
#include "../noise.glsl"
#include "procedural-terrain-common.glsl"

in vec3 vsout_samplePosition0;
in vec3 vsout_position0;
in vec2 vsout_uv0;
in vec3 vsout_normal0;

void main()
{
  vec3 normal = normalize(vsout_normal0);

  gbuffer_SetColour(vec4(vec3(0, 0, 0.2), 0.9));
  gbuffer_SetAmbient(texture2D(ambientMap, vsout_uv0) * ambient);
  gbuffer_SetPosition(vsout_position0);
  gbuffer_SetNormal(normal);
  gbuffer_SetRMAO(vec4(0.1, 0.1, 1, 1));
}
