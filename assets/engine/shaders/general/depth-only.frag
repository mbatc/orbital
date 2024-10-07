#version 430

#define OUT_COLOUR 0
#define OUT_AMBIENT 1
#define OUT_POSITION 2
#define OUT_NORMAL 3
#define OUT_RMAO 4
#define OUT_COUNT 5

layout(std140, binding=3) uniform Material {
  vec4 albedo;
  vec4 ambient;
  vec4 emissive;
  float roughness;
  float metalness;
  float padding[2];
};

layout(binding = 0) uniform sampler2D baseColourMap;
layout(binding = 1) uniform sampler2D ambientMap;
layout(binding = 2) uniform sampler2D emissiveMap;
layout(binding = 3) uniform sampler2D roughnessMap;
layout(binding = 4) uniform sampler2D metalnessMap;
layout(binding = 5) uniform sampler2D aoMap;
layout(binding = 6) uniform sampler2D alphaMap;
layout(binding = 7) uniform sampler2D normalMap;

in vec3 vsout_position0;
in vec2 vsout_uv0;
in mat3 vsout_tbnMat0;

out vec4 fragOut[OUT_COUNT];

void main()
{
  vec3 normal;
  normal = texture(normalMap, vsout_uv0).rgb;
  normal = normal * 2.0 - 1.0;   
  normal = normalize(vsout_tbnMat0 * normal); 

  fragOut[OUT_COLOUR] = texture2D(baseColourMap, vsout_uv0);
  fragOut[OUT_AMBIENT] = texture2D(ambientMap, vsout_uv0);
  fragOut[OUT_POSITION] = vec4(vsout_position0, 1);
  fragOut[OUT_NORMAL] = vec4(normal / 2 + vec3(0.5), 1);
  fragOut[OUT_RMAO] = vec4(
    roughness * texture2D(roughnessMap, vsout_uv0).x,
    metalness * texture2D(metalnessMap, vsout_uv0).x,
    texture2D(aoMap, vsout_uv0).x,
    1
  );
}
