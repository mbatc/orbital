#version 430

#include "../material-terrain.glsl"
#include "../gbuffer-out.glsl"
#include "../noise.glsl"

in vec3 vsout_position0;
in vec2 vsout_uv0;
in mat3 vsout_tbnMat0;
in float vsout_elevation0;

const vec3 colours[] = vec3[](
  vec3(0, 0, 1),
  vec3(0, 0, 1),
  vec3(1, 1, 0),
  vec3(0, 1, 0),
  vec3(0, 1, 0),
  vec3(0, 1, 0),
  vec3(0, 1, 0),
  vec3(1, 1, 1),
  vec3(1, 1, 1)
);

void main()
{
  vec3 normal;
  normal = texture(normalMap, vsout_uv0).rgb;
  normal = normal * 2.0 - 1.0;
  normal = normalize(vsout_tbnMat0 * normal);

  const vec3 colour = colours[clamp(int(vsout_elevation0 * colours.length()), 0, colours.length() - 1)];

  uint  seed  = 0x578437adU; // can be set to something else if you want a different set of random values
  float scale = 10;
  float noise = perlinNoise(vsout_uv0 * scale, 1, 6, 0.5, 2.0, seed); // multiple octaves
  noise = (noise + 1.0) * 0.5; // convert from range [-1, 1] to range [0, 1]

  gbuffer_SetColour(vec4(noise * colour, 1));
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
