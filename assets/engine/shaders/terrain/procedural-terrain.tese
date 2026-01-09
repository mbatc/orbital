#version 430

#include "../common.glsl"
#include "procedural-terrain-common.glsl"

layout (triangles, equal_spacing, ccw) in;

in vec3 tcsout_position0[];
in vec2 tcsout_uv0[];
in vec3 tcsout_normal0[];
in vec3 tcsout_tangent0[];

// Use vsout so that frag shaders don't need
// to be aware that the vertices are tesselated.
out vec3 vsout_position0;
out vec2 vsout_uv0;
out mat3 vsout_tbnMat0;

void main()
{
  vec3 position = 
    tcsout_position0[0] * gl_TessCoord.x +
    tcsout_position0[1] * gl_TessCoord.y +
    tcsout_position0[2] * gl_TessCoord.z;

  vsout_uv0 =
    tcsout_uv0[0] * gl_TessCoord.x +
    tcsout_uv0[1] * gl_TessCoord.y +
    tcsout_uv0[2] * gl_TessCoord.z;

  vec3 up = tcsout_normal0[0] * gl_TessCoord.x +
    tcsout_normal0[1] * gl_TessCoord.y +
    tcsout_normal0[2] * gl_TessCoord.z;

  float h = sampleTerrainHeight(vsout_uv0);
  vec3 displaced = position + up * h;

  float d = 1.0 / gl_TessLevelOuter[0];
  float dx = sampleTerrainHeight(vsout_uv0 + vec2(d, 0));
  float dy = sampleTerrainHeight(vsout_uv0 + vec2(0, d));
  vec3 normal = normalize(
    cross(
      displaced - (position + vec3(0, 0, d) + up * dy),
      displaced - (position + vec3(d, 0, 0) + up * dx)
    ));

  vec3 tangent = normalize(
    tcsout_tangent0[0] * gl_TessCoord.x +
    tcsout_tangent0[1] * gl_TessCoord.y +
    tcsout_tangent0[2] * gl_TessCoord.z);

  vec3 T = normalize(vec3(normalMatrix * vec4(tangent, 0.0)));
  vec3 N = normalize(vec3(normalMatrix * vec4(normal, 0.0)));

  // re-orthogonalize T with respect to N
  T = normalize(T - dot(T, N) * N);
  // then retrieve perpendicular vector B with the cross product of T and N
  vec3 B = cross(N, T);

  vsout_tbnMat0 = mat3(T, B, N);
  vsout_position0 = (modelMatrix * vec4(displaced, 1)).xyz;

  gl_Position = viewProjMatrix * vec4(vsout_position0, 1);
}
