#version 430

#include "common.h.glsl"
#include "postprocessinput.h.glsl"

out vec4 fragColour;

uniform float exposure;

in vec2 vsout_uv0;
in vec3 vsout_position0;

void main()
{
  const float gamma = 2.2;
  vec3 hdrColor = texture(sceneColour, vsout_uv0).rgb;
  // exposure tone mapping
  vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

  // gamma correction 
  mapped = pow(mapped, vec3(1.0 / gamma));

  fragColour = vec4(mapped, 1.0);
}
