#version 430

#include "../material-pbr.glsl"
#include "../gbuffer.glsl"
#include "../gbuffer-out.glsl"
#include "../common.glsl"

in vec3 vsout_position0;
in vec2 vsout_uv0;
in mat3 vsout_tbnMat0;

uniform vec3 absorption = vec3(1.8, 0.2, 0.2); // m^-1 * cm^-1 (water visible light)

void main()
{
  vec2 screenUV = getScreenUV(vsout_position0);
  vec3 surfacePosition = gbuffer_ReadPosition(screenUV);
  vec3 dir = vsout_position0 - surfacePosition;
  float pathLength = length(dir);
  vec3 transmittance = vec3(exp(-absorption * pathLength * 10000));

  gbuffer_SetColour(vec4(transmittance, 1));

  vec3 normal;
  normal = texture(normalMap, vsout_uv0).rgb;
  normal = normal * 2.0 - 1.0;
  normal = normalize(vsout_tbnMat0 * normal);

  gbuffer_SetAmbient(texture2D(ambientMap, vsout_uv0) * ambient);
  gbuffer_SetPosition(vsout_position0);
  gbuffer_SetNormal(normal);
  gbuffer_SetRMAO(vec4(
    roughness * texture2D(roughnessMap, vsout_uv0).x,
    metalness * texture2D(metalnessMap, vsout_uv0).x,
    texture2D(aoMap, vsout_uv0).x,
    1
  ));
}
