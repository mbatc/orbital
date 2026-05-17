#version 430

#include "../lighting-pbr.glsl"
#include "../gbuffer.glsl"

in vec2 vsout_uv0;
in vec3 vsout_position0;

out vec4 fragColour;

void main() {
  // Sample GBuffers
  vec3 base     = texture2D(G_BaseColour, vsout_uv0).xyz;
  vec3 position = texture2D(G_Position, vsout_uv0).xyz;
  vec3 rma      = texture2D(G_RMA, vsout_uv0).xyz;
  vec3 normal   = normalize(2 * (texture2D(G_Normal, vsout_uv0).xyz - vec3(0.5)));

  float roughness = rma.x;
  float metalness = rma.y;
  float ao        = rma.z;

  vec3 camPos = invViewMatrix[3].xyz;

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

    float shadowing = CalcShadowing(lightDir, lightData[i].shadowMapRange, position, normal);
    Lo += lighting * shadowing;
  }

  vec3 ambient = ambientLight * base * ao;
  vec3 color   = ambient + Lo;
  fragColour = vec4(color, 1);
}
