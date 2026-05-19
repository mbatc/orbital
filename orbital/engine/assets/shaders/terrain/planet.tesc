#version 430

#include "../common.glsl"

layout (vertices=3) out;

in vec3 vsout_ts_position0[];
in vec2 vsout_ts_uv0[];
in vec3 vsout_ts_normal0[];
in vec3 vsout_ts_tangent0[];

out vec3 tcsout_position0[];
out vec2 tcsout_uv0[];
out vec3 tcsout_normal0[];
out vec3 tcsout_tangent0[];

float GetTessLevel(float Distance0, float Distance1)
{
  float AvgDistance = (Distance0 + Distance1) / 2.0;

  if (AvgDistance <= 2.0) {
    return 10.0;
  }
  else if (AvgDistance <= 5.0) {
    return 7.0;
  }
  else {
    return 3.0;
  }
}

void main()
{
  vec3 cameraPos = invViewMatrix[3].xyz;
  
  tcsout_position0[gl_InvocationID] = vsout_ts_position0[gl_InvocationID];
  tcsout_uv0[gl_InvocationID] = vsout_ts_uv0[gl_InvocationID];
  tcsout_normal0[gl_InvocationID] = vsout_ts_normal0[gl_InvocationID];
  tcsout_tangent0[gl_InvocationID] = vsout_ts_tangent0[gl_InvocationID];

  if (gl_InvocationID == 0)
  {
    gl_TessLevelOuter[0] = 64;
    gl_TessLevelOuter[1] = 64;
    gl_TessLevelOuter[2] = 64;

    gl_TessLevelInner[0] = 64;
  }
}
