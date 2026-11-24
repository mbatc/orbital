#version 430

#include "../lighting-pbr.glsl"
#include "../gbuffer.glsl"

in vec2 vsout_uv0;
in vec3 vsout_position0;

out vec4 fragColour;

const float rotations[9] = float[](
    4.526359, // 259.3 deg
    1.558603, //  89.3 deg
    1.111840, //  63.7 deg
    0.523038, //  30.0 deg
    5.905415, // 338.4 deg
    2.370467, // 135.8 deg
    3.014585, // 172.7 deg
    5.428435, // 311.0 deg
    3.632439  // 208.1 deg
);

float getRand(vec2 pixel) {
  ivec2 randCoord = ivec2(mod(pixel, vec2(3, 3)));
  return rotations[randCoord.y * 3 + randCoord.x];
}

void main() {
  // Sample GBuffers
  vec3 base     = gbuffer_ReadColour(vsout_uv0).xyz;
  vec3 position = gbuffer_ReadPosition(vsout_uv0);
  vec3 emissive = gbuffer_ReadAmbient(vsout_uv0);
  vec3 rma      = gbuffer_ReadRMA(vsout_uv0);
  vec3 normal   = gbuffer_ReadNormal(vsout_uv0);

  float rand    = getRand(gbuffer_PixelCoordinate(vsout_uv0));
  float roughness = rma.x;
  float metalness = rma.y;
  float ao        = rma.z;

  vec3 camPos = getCameraPosition();

  vec3 N = normal;
  vec3 V = normalize(camPos - position);
  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, base, metalness);

  // Calculate lighting for each light in 'lightData'
  // TODO: Instanced render. Use geometry shader to generate bounding geometry for each light
  vec3 Lo = vec3(0);
  vec3 ambientLight = vec3(0);
  for (int i = 0; i < lightData.length(); ++i) {
    vec3 lighting = vec3(0);
    vec3 lightDir = lightData[i].direction;
    switch (lightData[i].type)
    {
    case LightType_Sun:
      lighting = CalcLighting(
        lightData[i].colour.xyz,
        -lightData[i].direction,
        lightData[i].strength,
        V,
        F0,
        base,
        N,
        roughness,
        metalness
      );
      ambientLight += lightData[i].ambient;
      break;

    case LightType_Point:
      lightDir = normalize(position - lightData[i].position);
      lighting = CalcPointLight(
        lightData[i].colour.xyz,
        lightData[i].position,
        lightData[i].attenuation,
        lightData[i].strength,
        V,
        position,
        F0,
        base,
        N,
        roughness,
        metalness
      );
      break;
    
    case LightType_Spot:
      lightDir = normalize(position - lightData[i].position);
      lighting = CalcSpotLight(
        lightData[i].colour.xyz,
        lightData[i].position,
        lightData[i].direction,
        lightData[i].attenuation,
        lightData[i].innerCutoff,
        lightData[i].outerCutoff,
        lightData[i].strength,
        V,
        position,
        F0,
        base,
        N,
        roughness,
        metalness
      );
      break;
    }

    float shadowing = CalcShadowing(lightDir, lightData[i].shadowMapRange, position, normal, rand);
    Lo += lighting * shadowing;
  }

  vec3 ambient = ambientLight * base * ao + emissive;
  vec3 color   = ambient + Lo;
  fragColour = vec4(color, 1);
}
