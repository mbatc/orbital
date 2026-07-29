#version 430

#include "../../common.glsl"
#include "../../postprocessinput.glsl"

in vec3 vsout_position0;
in vec2 vsout_uv0;
in mat3 vsout_tbnMat0;

uniform float mieConstant;
uniform float rayleighConstant;
uniform float innerRadius;
uniform float outerRadius;
uniform vec3 sunDirection;
uniform float sunIntensity;

out vec4 fragColour;

bool rayOnSphere(vec3 rayPos, vec3 rayDir, vec3 sphereCenter, float radius, out float t0, out float t1) {
  vec3  toCenter        = sphereCenter - rayPos;
  float distToCenter    = length(toCenter);
  float distToMidpoint  = dot(rayDir, toCenter);
  float distMidToCenter = distToCenter * distToCenter - distToMidpoint * distToMidpoint;
  float depth2          = radius * radius - distMidToCenter;
  if (depth2 <= 0)
    return false;
  float depth = sqrt(depth2);
  t0 = distToMidpoint - depth;
  t1 = distToMidpoint + depth;
  return true;
}

void main() {
  vec3 wavelengths = pow(vec3(1) - vec3(0.650, 0.57, 0.475), vec3(4));

  float depth = 0;
  vec3 cameraPosition = getCameraPosition();
  vec3 viewDirection = normalize(vsout_position0 - cameraPosition);
  vec3 normal = vec3(0);

  float t0;
  float t1;
  float atmosEnter = 0;
  float atmosEnd = 0;
  if (rayOnSphere(cameraPosition, viewDirection, getModelPosition(), outerRadius, t0, t1))
  {
    atmosEnter = t0;
    atmosEnd = t1;
    if (rayOnSphere(cameraPosition, viewDirection, getModelPosition(), innerRadius, t0, t1))
      atmosEnd = t0;
  }

  normal = normalize((cameraPosition + viewDirection * atmosEnter) - getModelPosition());

  vec3 scenePosition = pps_ReadPosition(getScreenUV(vsout_position0));
  atmosEnd = min(atmosEnd, length(scenePosition - cameraPosition));
  depth = max(0, atmosEnd - atmosEnter);
  depth = depth * depth;

  fragColour = vec4(wavelengths, depth) * max(0, dot(normal, -sunDirection)) * sunIntensity;
  // fragColour = vec4(normal, 1);
}
