#version 430

#include "../material-pbr.glsl"
#include "../gbuffer.glsl"
#include "../common.glsl"

in vec3 vsout_position0;
in vec2 vsout_uv0;

uniform vec3 absorption = vec3(1.8, 0.2, 0.2); // m^-1 * cm^-1 (water visible light)

out vec3 transmittance;

void main()
{
  vec2 screenUV = getScreenUV(vsout_position0);
  vec3 surfacePosition = gbuffer_ReadPosition(screenUV);

  vec3 dir = vsout_position0 - surfacePosition;
  float pathLength = length(dir);

  transmittance = exp(-absorption * pathLength * 100);
}
