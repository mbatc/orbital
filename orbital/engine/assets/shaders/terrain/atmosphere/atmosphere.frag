#version 430

#include "../../material-pbr.glsl"
#include "../../gbuffer-out.glsl"
#include "../../common.glsl"


in vec3 vsout_position0;
in vec2 vsout_uv0;
in mat3 vsout_tbnMat0;

bool rayOnSphere(vec3 rayPos, vec3 rayDir, vec3 sphereCenter, float radius, out float t0, out float t1) {
  vec3  toCenter       = sphereCenter - rayPos;
  float distToCenter   = length(toCenter);
  float distToMidpoint = dot(rayDir, toCenter);
  if (distToMidpoint <= 0)
    return false;
  float distMidToCenter = distToCenter * distToCenter - distToMidpoint * distToMidpoint;
  if (distMidToCenter <= 0)
    return false;
  float depth2 = radius * radius - distMidToCenter;
  if (depth2 <= 0)
    return false;
  float depth = sqrt(depth2);
  t0 = distToMidpoint - depth;
  t1 = distToMidpoint + depth;
  return true;
}

void main()
{
  float t0;
  float t1;
  float depth = 0;
  if (rayOnSphere(getCameraPosition(), normalize(vsout_position0 - getCameraPosition()), getModelPosition(), 1.025, t0, t1))
  {
    float innerT0 = t0;
    float innerT1 = t1;
    if (rayOnSphere(getCameraPosition(), normalize(vsout_position0 - getCameraPosition()), getModelPosition(), 1, innerT0, innerT1))
      depth = (innerT0 - t0) / 0.5;
    else
      depth = (t1 - t0) / 0.5;
    depth = depth * depth;
  }

  vec3 normal;
  normal = texture(normalMap, vsout_uv0).rgb;
  normal = normal * 2.0 - 1.0;
  normal = normalize(vsout_tbnMat0 * normal);

  gbuffer_SetColour(vec4(0.2, 0.6, 0.9, depth) * 5);
  gbuffer_SetAmbient(texture2D(ambientMap, vsout_uv0) * ambient);
  gbuffer_SetPosition(vsout_position0);
  gbuffer_SetNormal(normal);
  gbuffer_SetRMAO(vec4(
    roughness * texture2D(roughnessMap, vsout_uv0).x,
    metalness * texture2D(metalnessMap, vsout_uv0).x,
    texture2D(aoMap, vsout_uv0).x,
    1
  ));
}
