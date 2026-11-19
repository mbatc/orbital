#version 430

#include "../common.glsl"
#include "../postprocessinput.glsl"

layout(binding = 7) uniform sampler2D ssaoTex;

in vec2 vsout_uv0;
in vec3 vsout_position0;

out vec4 fragColour;

void main()
{
  fragColour = vec4(texture(sceneColourTex, vsout_uv0).rgb * (texture(ssaoTex, vsout_uv0).r), 1);
}
