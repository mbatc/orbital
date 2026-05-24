#version 430

#include "../material-pbr.glsl"
#include "../gbuffer-out.glsl"
#include "../common.glsl"

layout(binding = 8) uniform sampler2D transmittanceMap;

in vec3 vsout_position0;
in vec2 vsout_uv0;
in mat3 vsout_tbnMat0;

out vec4 fragColour;

void main()
{
  vec2 screenUV = getScreenUV(vsout_position0);
  vec3 transmittance = texture(transmittanceMap, screenUV).rgb;

  vec3 normal;
  normal = texture(normalMap, vsout_uv0).rgb;
  normal = normal * 2.0 - 1.0;
  normal = normalize(vsout_tbnMat0 * normal);

  fragColour = vec4(transmittance, 1);
}
