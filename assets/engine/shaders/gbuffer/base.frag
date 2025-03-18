#version 430

#include "../material-pbr.glsl"
#include "../gbuffer-out.glsl"

in vec3 vsout_position0;
in vec2 vsout_uv0;
in mat3 vsout_tbnMat0;

void main()
{
  vec3 normal;
  normal = texture(normalMap, vsout_uv0).rgb;
  normal = normal * 2.0 - 1.0;   
  normal = normalize(vsout_tbnMat0 * normal); 

  gbuffer_SetColor(texture2D(baseColourMap, vsout_uv0) * albedo);
  gbuffer_SetAmbient(texture2D(ambientMap, vsout_uv0) * ambient);
  gbuffer_SetPosition(vec4(vsout_position0, 1));
  gbuffer_SetNormal(vec4(normal / 2 + vec3(0.5), 1));
  gbuffer_SetRMAO(vec4(
    roughness * texture2D(roughnessMap, vsout_uv0).x,
    metalness * texture2D(metalnessMap, vsout_uv0).x,
    texture2D(aoMap, vsout_uv0).x,
    1
  ));
}
