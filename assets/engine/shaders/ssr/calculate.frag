#version 430

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

layout(std140, binding=2) buffer lights {
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

// ----------------------------------

uniform float maxDistance;
uniform float resolution;
uniform int   steps;
uniform float thickness;

uniform ivec2 outputSize;

in vec2 vsout_uv0;
in vec3 vsout_position0;

out vec4 fragColour;

float calculateUVFalloff(float a) {
  return clamp(1 - pow(abs(a - 0.5f) * 2.0f, 2.0f), 0, 1);
}

void main() {
  float fragDepth = texture(sceneDepthTex, vsout_uv0).r;

  if (fragDepth == 1) {
    fragColour = vec4(0, 0, 0, 0);
    return;
  }

  // Calculate start and end position of the reflection ray in world space
  vec3 rayStart   = texture(positionTex, vsout_uv0).xyz;
  vec3 camPos     = invViewMatrix[3].xyz;
  vec3 fromCamera = rayStart - camPos;
  vec3 unitPositionFrom = normalize(fromCamera);

  vec3 camDir     = normalize(-invViewMatrix[2].xyz);
  vec3 normal     = normalize(2 * (texture(normalTex, vsout_uv0).xyz - vec3(0.5)));
  vec3 rayDir     = normalize(reflect(fromCamera, normal));
  float rayLen    = maxDistance;
  vec3 rayEnd     = rayStart + rayDir * rayLen;

  float distanceToCameraPlane = dot(camDir, rayEnd - camPos);
  if (distanceToCameraPlane <= 0) {
    float denom = dot(-camDir, rayDir);
    rayEnd = rayEnd - rayDir * abs(distanceToCameraPlane - 0.01) / denom;
  }

  // Transform the start/end to screen space
  vec4 screenStart = viewProjMatrix * vec4(rayStart, 1);
  vec4 screenEnd   = viewProjMatrix * vec4(rayEnd, 1);

  screenStart    /= screenStart.w;
  screenStart.xy  = screenStart.xy * 0.5 + 0.5;
  screenStart.xy *= outputSize;

  screenEnd    /= screenEnd.w;
  screenEnd.xy  = screenEnd.xy * 0.5 + 0.5;
  screenEnd.xy *= outputSize;
  
  float deltaX   = screenEnd.x - screenStart.x;
  float deltaY   = screenEnd.y - screenStart.y;
  float useX     = abs(deltaX) >= abs(deltaY) ? 1.0 : 0.0;
  float delta    = mix(abs(deltaY), abs(deltaX), useX) * clamp(resolution, 0, 1);
  vec2 increment = vec2(deltaX, deltaY) / max(delta, 0.0001);
  
  float search0 = 0.0f;
  float search1  = 0.0f;
  int hit0 = 0;
  int hit1 = 0;
  vec2 frag = screenStart.xy;
  vec2 uv = vsout_uv0;
  vec4 reflectedPosition = texture(positionTex, vsout_uv0);
  
  float startDistance = dot(camDir, rayStart - camPos);
  float endDistance   = dot(camDir, rayEnd - camPos);
  float viewDistance  = startDistance;
  float depth = 0;

  for (int i = 0; i < int(delta); ++i) {
    frag += increment;
    uv = frag / outputSize;
    if (uv.x < 0 || uv.y < 0 || uv.x > 1 || uv.y > 1) {
      break;
    }
    reflectedPosition = texture(positionTex, uv);
  
    search1 = mix((frag.y - screenStart.y) / deltaY, (frag.x - screenStart.x) / deltaX, useX);
    viewDistance = (startDistance * endDistance) / mix(endDistance, startDistance, search1);
    depth = viewDistance - dot(camDir, reflectedPosition.xyz - camPos);
  
    float adjustedThickness = max(thickness, viewDistance * thickness) / resolution;

    if (depth > 0 && depth < adjustedThickness) {
      hit0 = 1;
      break;
    }
  
    search0 = search1;
  }
  
  search1 = search0 + (search1 - search0) / 2;
  
  if (hit0 > 0) {
    for (int i = 0; i < steps; ++i) {
      frag = mix(screenStart.xy, screenEnd.xy, search1);
      uv   = frag / outputSize;
      reflectedPosition = texture(positionTex, uv);
  
      viewDistance = (startDistance * endDistance) / mix(endDistance, startDistance, search1);
      depth = viewDistance - dot(camDir, reflectedPosition.xyz - camPos);
  
      float adjustedThickness = max(thickness, viewDistance * thickness) / resolution;

      if (depth > 0 && depth < adjustedThickness) {
        hit1 = 1;
        search1 = search0 + ((search1 - search0) / 2);
      } else {
        float temp = search1;
        search1 = search1 + ((search1 - search0) / 2);
        search0 = temp;
      }
    }
  }
  
  float vis = 1
    * hit1
    * calculateUVFalloff(uv.x)
    * calculateUVFalloff(uv.y)
    * reflectedPosition.w
    * (1 - max(dot(-unitPositionFrom, rayDir), 0))
    * (1 - clamp(depth / thickness, 0, 1))
    * (1 - clamp(length(reflectedPosition.xyz - rayStart) / rayLen, 0, 1))
  ;

  fragColour = vec4(uv, 0, vis);
}
