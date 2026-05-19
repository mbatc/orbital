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
out vec3 vsout_normal0;

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

  vec3 tilePosition   = vec3(terrainTile.sampleTransform * vec4(position, 1));
  vec3 samplePosition = normalize(tilePosition);
  vec3 up             = samplePosition;

  vsout_position0       = samplePosition.xyz;
  vsout_samplePosition0 = samplePosition;
  vsout_normal0         = up;
  gl_Position = viewProjMatrix * vec4(vsout_position0, 1);
}
