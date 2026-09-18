#version 430

#include "../common.glsl"

layout(binding = 0) uniform samplerCube texture0;

in vec3 vsout_direction0;

out vec4 psout_colour0;

void main()
{
  vec3 colour = texture(texture0, vsout_direction0).rgb;
  // vec3 streched = (texture(texture0, vsout_direction0).rgb - vec3(0.5)) * 5 + vec3(0.5);
  psout_colour0 = vec4(colour, 1);
}
