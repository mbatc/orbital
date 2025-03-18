#version 430

#include "../lighting-pbr.glsl"
#include "../gbuffer.h.glsl"

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

#define MIN_ATTEN 0.0001

float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a      = roughness * roughness;
  float a2     = a * a;
  float NdotH  = max(dot(N, H), 0.0);
  float NdotH2 = NdotH*NdotH;

  float num   = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float num   = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2  = GeometrySchlickGGX(NdotV, roughness);
  float ggx1  = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 CalcLighting(vec3 colour, vec3 lightDirection, float attenuation, vec3 viewDirection, vec3 F0, vec3 albedo, vec3 normal, float roughness, float metalness) {    
  // Calculate radiance
  vec3 halfway  = normalize(viewDirection + lightDirection);
  vec3 radiance = colour * attenuation;

  // Cook-torrance brdf
  float NDF = DistributionGGX(normal, halfway, roughness);
  float G   = GeometrySmith(normal, viewDirection, lightDirection, roughness);
  vec3  F   = FresnelSchlick(max(dot(halfway, viewDirection), 0.0), F0);
  
  vec3  numerator   = NDF * G * F;
  float denominator = 4.0 * max(dot(normal, viewDirection), 0.0) * max(dot(normal, lightDirection), 0.0) + 0.0001;
  vec3  specular    = numerator / denominator;
  
  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metalness;

  // Add to outgoing radiance Lo
  float NdotL = max(dot(normal, lightDirection), 0.0);

  return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 CalcPointLight(vec3 colour, vec3 lightPosition, vec3 attenFactor, float strength, vec3 viewDirection, vec3 fragPos, vec3 F0, vec3 albedo, vec3 normal, float roughness, float metalness) {
  vec3 lightDirection = lightPosition - fragPos;
  float dist = length(lightDirection);
  lightDirection /= dist;
  float effectiveDist = dist / strength;
  float attenuation = 1.0 / max(MIN_ATTEN, (attenFactor.x + attenFactor.y * effectiveDist + attenFactor.z * effectiveDist * effectiveDist));
  return CalcLighting(colour, lightDirection, attenuation, viewDirection, F0, albedo, normal, roughness, metalness);
}

vec3 CalcSpotLight(vec3 colour, vec3 lightPosition, vec3 lightDirection, vec3 attenFactor, float innerCutoff, float outerCutoff, float strength, vec3 viewDirection, vec3 fragPos, vec3 F0, vec3 albedo, vec3 normal, float roughness, float metalness) {
  vec3 lightToFrag =  lightPosition - fragPos;
  float dist = length(lightToFrag);
  lightToFrag /= dist;
  float effectiveDist = dist / strength;
  float attenuation = 1.0 / max(MIN_ATTEN, (attenFactor.x + attenFactor.y * effectiveDist + attenFactor.z * effectiveDist * effectiveDist));
  vec3 Lo = CalcLighting(colour, lightToFrag, attenuation, viewDirection, F0, albedo, normal, roughness, metalness);

  float theta     = dot(lightToFrag, -lightDirection);
  float epsilon   = innerCutoff - outerCutoff;
  float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

  return Lo * intensity;
}

float CalcShadowing(vec3 lightDir, uvec2 shadowMapRange, vec3 fragPos, vec3 fragNormal) {
  float bias = max(0.005 * (1.0 - dot(fragNormal, lightDir)), 0.001);
  if (shadowMapRange.y - shadowMapRange.x == 6) {
    // Cube-map shadow
    vec3 cubeCoord = SampleCube(lightDir);
    uint face = shadowMapRange.x + uint(cubeCoord.z);

    vec4 projectedPixel = shadowData[face].vp * vec4(fragPos, 1);
    vec3 projectedCoord = projectedPixel.xyz / projectedPixel.w;
    projectedCoord = projectedCoord * 0.5 + 0.5;

    float shadowDepth = textureLod(ShadowAtlas, vec3(projectedCoord.xy, shadowData[face].layer), shadowData[face].level).r;
    return projectedCoord.z - bias > shadowDepth ? 0.0f : 1.0f;
  } else {
    for (uint i = shadowMapRange.x; i < shadowMapRange.y; ++i) {
      vec4 projectedPixel = shadowData[i].vp * vec4(fragPos, 1);
      vec3 projectedCoord = projectedPixel.xyz / projectedPixel.w;
      projectedCoord = projectedCoord * 0.5 + 0.5;

      if (projectedCoord.x >= 0 && projectedCoord.x <= 1 && projectedCoord.y >= 0 && projectedCoord.y <= 1)
      {
        float shadowDepth = textureLod(ShadowAtlas, vec3(projectedCoord.xy, shadowData[i].layer), shadowData[i].level).r;
        if (projectedCoord.z < 0 || projectedCoord.z > 1)
        {
          if (shadowDepth != 1)
            return 0;
        } 
        else if (projectedCoord.z - bias > shadowDepth)
          return 0;
        // if (projectedCoord.z - bias > shadowDepth || ) {
        //   return shadowDepth;
        // }
      }
    }
  }
  return 1;
}
