#version 430

// TODO: Handle Includes
// #include "../common.h.glsl"

// common.h.glsl
// -----------------------------

#define LightType_Sun   0
#define LightType_Point 1
#define LightType_Spot  2

struct LightBuffer {
  vec3 position;
  int  type;

  vec3 colour;
  float padding0;

  vec3 ambient;
  float strength;

  vec3  attenuation;
  float innerCutoff;

  vec3  direction;
  float outerCutoff;
  
  uvec2 shadowMapRange;
  float padding1;
  float padding2;
};

struct ShadowMapData {
  mat4 vp;
  int layer;
  int level;
  float padding1;
  float padding2;
};

layout(std140, binding=0) uniform Camera {
  mat4 viewProjMatrix;
  mat4 viewMatrix;
  mat4 projMatrix;
  mat4 invViewProjMatrix;
  mat4 invViewMatrix;
  mat4 invProjMatrix;
};

layout(std140, binding=1) uniform Model {
  mat4 modelMatrix;
  mat4 normalMatrix;
  mat4 mvpMatrix;
};

layout(std140, binding=2) buffer Lights {
  LightBuffer lightData[];
};

layout(std140, binding=3) buffer Shadows {
  ShadowMapData shadowData[];
};

// -----------------------------

// gbuffer.h.glsl
// -----------------------------

layout(binding = 0) uniform sampler2D G_BaseColour;
layout(binding = 1) uniform sampler2D G_Ambient;
layout(binding = 2) uniform sampler2D G_Position;
layout(binding = 3) uniform sampler2D G_Normal;
layout(binding = 4) uniform sampler2D G_RMA;
layout(binding = 5) uniform sampler2DArray ShadowAtlas;

// -----------------------------

vec3 SampleCube(const vec3 dir)
{
  vec3 absDir = abs(dir);
  vec2 uv = vec2(0);
  float face = 0;
  float ma = 0;

  if (absDir.z >= absDir.x && absDir.z >= absDir.y) {
    face = dir.z < 0.0 ? 5.0 : 4.0;
    ma = 0.5 / absDir.z;
    uv = vec2(dir.z < 0.0 ? -dir.x : dir.x, -dir.y);
  } else if (absDir.y >= absDir.x) {
    face = dir.y < 0.0 ? 3.0 : 2.0;
    ma = 0.5 / absDir.y;
    uv = vec2(dir.x, dir.y < 0.0 ? -dir.z : dir.z);
  } else {
    face = dir.x < 0.0 ? 1.0 : 0.0;
    ma = 0.5 / absDir.x;
    uv = vec2(dir.x < 0.0 ? dir.z : -dir.z, -dir.y);
  }

  return vec3(uv * ma + 0.5, face);
}

in vec2 vsout_uv0;
in vec3 vsout_position0;

out vec4 fragColour;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0);

#define MIN_ATTEN 0.0001

vec3 CalcLighting(vec3 colour, vec3 lightDirection, float attenuation, vec3 viewDirection, vec3 F0, vec3 albedo, vec3 normal, float roughness, float metalness);
vec3 CalcPointLight(vec3 colour, vec3 lightPosition, vec3 attenFactor, float strength, vec3 viewDirection, vec3 fragPos, vec3 F0, vec3 albedo, vec3 normal, float roughness, float metalness);
vec3 CalcSpotLight(vec3 colour, vec3 lightPosition, vec3 lightDirection, vec3 attenFactor, float innerCutoff, float outerCutoff, float strength, vec3 viewDirection, vec3 fragPos, vec3 F0, vec3 albedo, vec3 normal, float roughness, float metalness);
float CalcShadowing(vec3 lightDir, uvec2 shadowMapRange, vec3 fragPos, vec3 fragNormal);

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

      float shadowDepth = textureLod(ShadowAtlas, vec3(projectedCoord.xy, shadowData[i].layer), shadowData[i].level).r;

      if (projectedCoord.z - bias > shadowDepth) {
        return 0;
      }
    }
  }
  return 1.0f;
}
