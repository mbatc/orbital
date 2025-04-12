#version 430

#include "../material-pbr.glsl"
#include "../gbuffer-out.glsl"
#include "../noise.glsl"

in vec3 vsout_position0;
in vec2 vsout_uv0;
in mat3 vsout_tbnMat0;

void main()
{
  vec3 normal;
  normal = texture(normalMap, vsout_uv0).rgb;
  normal = normal * 2.0 - 1.0;
  normal = normalize(vsout_tbnMat0 * normal);

  uint seed = 0x578437adU; // can be set to something else if you want a different set of random values
  float scale = 10;
  float noise = perlinNoise(vsout_uv0 * scale, 1, 6, 0.5, 2.0, seed); // multiple octaves
  noise = (noise + 1.0) * 0.5; // convert from range [-1, 1] to range [0, 1]

  gbuffer_SetColour(noise * albedo);
  // gbuffer_SetColour(vec4(vsout_uv0, 0, 1));
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
