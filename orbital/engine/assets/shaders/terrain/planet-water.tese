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
out vec3 vsout_samplePosition0;
out vec3 vsout_position0;
out vec2 vsout_uv0;
out mat3 vsout_tbnMat0;

void main()
{
  vec3 position = 
    tcsout_position0[0] * gl_TessCoord.x +
    tcsout_position0[1] * gl_TessCoord.y +
    tcsout_position0[2] * gl_TessCoord.z;

  vec3 tilePosition   = vec3(terrainTile.sampleTransform * vec4(position, 1));
  vec3 samplePosition = normalize(tilePosition);
  vec3 up             = samplePosition;
  vec3 normal = up;
  vec3 tangent = cross(up, vec3(0, 1, 0));
  vec3 T = normalize(vec3(normalMatrix * vec4(tangent, 0.0)));
  vec3 N = normalize(vec3(normalMatrix * vec4(normal, 0.0)));
  // re-orthogonalize T with respect to N
  T = normalize(T - dot(T, N) * N);
  // then retrieve perpendicular vector B with the cross product of T and N
  vec3 B = cross(N, T);

  vsout_uv0 =
    tcsout_uv0[0] * gl_TessCoord.x +
    tcsout_uv0[1] * gl_TessCoord.y +
    tcsout_uv0[2] * gl_TessCoord.z;
  vsout_tbnMat0         = mat3(T, B, N);
  vsout_position0       = (modelMatrix * vec4(samplePosition.xyz, 1)).xyz;
  vsout_samplePosition0 = samplePosition;
  gl_Position           = viewProjMatrix * vec4(vsout_position0, 1);
}
