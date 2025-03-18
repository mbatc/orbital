#version 430

#include "../common.h.glsl"

layout(binding = 0) uniform samplerCube texture0;

in vec3 vsout_direction0;

out vec4 psout_colour0;

void main()
{
  psout_colour0 = vec4(texture(texture0, vsout_direction0).rgb, 1);
}
