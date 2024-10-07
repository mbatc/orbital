#version 430

#define LightType_Directional 0
#define LightType_Point 0
#define LightType_Spot 0

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
  float coneCutoff;

  ivec2 shadowMapRange;
  float padding1;
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

layout(std140, binding = 3) buffer lights {
  LightBuffer lightData[];
};

// postprocessinput.h.glsl
// -----------------------------

layout(binding = 0) uniform sampler2D sceneColourTex;
layout(binding = 1) uniform sampler2D sceneDepthTex;
layout(binding = 2) uniform sampler2D baseColourTex;
layout(binding = 3) uniform sampler2D ambientTex;
layout(binding = 4) uniform sampler2D positionTex;
layout(binding = 5) uniform sampler2D normalTex;
layout(binding = 6) uniform sampler2D RMATex;

layout(binding = 7) uniform sampler2D reflectionUV;
layout(binding = 8) uniform sampler2D blurredSceneColourTex;
layout(binding = 9) uniform sampler2D BRDFLut;

// ----------------------------------

in vec2 vsout_uv0;
in vec3 vsout_position0;

out vec4 fragColour;

// TODO: Refactor PBR functions into common header
const float PI = 3.14159265359;

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

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
  // Sample GBuffers
  vec4 scene    = texture2D(sceneColourTex, vsout_uv0);
  vec3 base     = texture2D(baseColourTex, vsout_uv0).xyz;
  vec3 position = texture2D(positionTex, vsout_uv0).xyz;
  vec3 rma      = texture2D(RMATex, vsout_uv0).xyz;
  vec3 normal   = normalize(2 * (texture2D(normalTex, vsout_uv0).xyz - vec3(0.5)));
  
  float roughness = rma.x;
  float metalness = rma.y;
  float ao        = rma.z;
  
  vec3 camPos = invViewMatrix[3].xyz;

  vec3 N = normal;
  vec3 V = normalize(camPos - position);
  vec3 F0 = vec3(0.04); 
  F0 = mix(F0, base, metalness);

  vec3 F   = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
  // vec2 brdf  = texture(BRDFLut, vec2(max(dot(N, V), 0.0), roughness)).rg;
  vec2 brdf = vec2(1, 0);

  // Sample reflections
  vec4 uv      = textureLod(reflectionUV, vsout_uv0, 0);
  vec4 sharp   = textureLod(sceneColourTex, uv.xy, 0);
  vec4 blurred = textureLod(blurredSceneColourTex, uv.xy, 0);
  vec3 reflection = mix(sharp.xyz, blurred.xyz, roughness) * (F * brdf.x + brdf.y);

  fragColour = vec4(scene.xyz + reflection * uv.a, scene.a);
}
