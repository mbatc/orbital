#version 430

#include "../../common.glsl"

in layout(location = 0) vec3 position0;
in layout(location = 2) vec2 uv0;
in layout(location = 3) vec3 normal0;
in layout(location = 4) vec3 tangent0;

uniform float mieConstant;
uniform float rayleighConstant;
uniform float innerRadius;
uniform float outerRadius;
uniform vec3 sunDirection;
uniform float sunIntensity;

out vec3 vsout_position0;
out vec2 vsout_uv0;
out mat3 vsout_tbnMat0;

void main()
{
  const float innerRadius2 = innerRadius * innerRadius;
  const float outerRadius2 = outerRadius * outerRadius;
  const float scale = 1 / (outerRadius - innerRadius);
  const float scaleDepth = (innerRadius + outerRadius) / 2;
  const float scaleOverScaleDepth = scale / scaleDepth;
  const float cameraHeight = length(getCameraPosition());
  const float krESum = sunIntensity * rayleighConstant;
  const float kmESun = sunIntensity * mieConstant;
  const float kr4PI = rayleighConstant * 4 * PI;
  const float km4PI = mieConstant * 4 * PI;
  const int nSamples = 2;
  const float fSamples = 2.0;

  vsout_uv0 = vec2(uv0.x, 1 - uv0.y); // Flip on Y for OpenGL
  vsout_position0 = (modelMatrix * vec4(position0, 1)).xyz;
  // vec3 rayleighColour = vec3(0, 0, 0);

  // Get the ray from the camera to the vertex, and its length (which is the far point of the ray passing through the atmosphere)
  // vec3 camPos = getCameraPosition();
  // vec3 v3Ray = position0 - camPos;
  // float fFar = length(v3Ray);
  // v3Ray /= fFar;

  // Calculate the ray's starting position, then calculate its scattering offset
  // vec3 v3Start = camPos;
  // float fHeight = length(v3Start);
  // float fDepth = exp(scaleOverScaleDepth * (innerRadius - cameraHeight));
  // float fStartAngle = dot(v3Ray, v3Start) / fHeight;
  // float fStartOffset = fDepth*scale(fStartAngle);
  // vsout_rayleighColour = rayleighColour * fKmESun;

  vec3 T = normalize(vec3(normalMatrix * vec4(tangent0, 0.0)));
  vec3 N = normalize(vec3(normalMatrix * vec4(normal0, 0.0)));
  // re-orthogonalize T with respect to N
  T = normalize(T - dot(T, N) * N);
  // then retrieve perpendicular vector B with the cross product of T and N
  vec3 B = cross(N, T);
  vsout_tbnMat0 = mat3(T, B, N);

  gl_Position = mvpMatrix * vec4(position0, 1);
}
