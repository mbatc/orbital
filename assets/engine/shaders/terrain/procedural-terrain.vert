#version 430

#include "../common.glsl"

in layout(location = 0) vec3 position0;
in layout(location = 2) vec2 uv0;
in layout(location = 3) vec3 normal0;
in layout(location = 4) vec3 tangent0;

out vec3 vsout_ts_position0;
out vec2 vsout_ts_uv0;
out vec3 vsout_ts_normal0;
out vec3 vsout_ts_tangent0;

void main()
{
  vsout_ts_uv0       = vec2(uv0.x, 1 - uv0.y); // Flip on Y for OpenGL
  vsout_ts_position0 = position0; // Transformed in the evaluation shader
  vsout_ts_normal0   = normal0;
  vsout_ts_tangent0  = tangent0;
}
