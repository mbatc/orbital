#version 430

#include "../common.h.glsl"

in layout(location = 0) vec3 position0;
in layout(location = 2) vec2 uv0;
in layout(location = 3) vec3 normal0;
in layout(location = 4) vec3 tangent0;

out vec3 vsout_position0;
out vec2 vsout_uv0;
out mat3 vsout_tbnMat0;

void main()
{
  vsout_uv0 = vec2(uv0.x, 1 - uv0.y); // Flip on Y for OpenGL
  vsout_position0 = (modelMatrix * vec4(position0, 1)).xyz;
  
  vec3 T = normalize(vec3(normalMatrix * vec4(tangent0, 0.0)));
  vec3 N = normalize(vec3(normalMatrix * vec4(normal0, 0.0)));
  // re-orthogonalize T with respect to N
  T = normalize(T - dot(T, N) * N);
  // then retrieve perpendicular vector B with the cross product of T and N
  vec3 B = cross(N, T);
  vsout_tbnMat0 = mat3(T, B, N);

  gl_Position = mvpMatrix * vec4(position0, 1);
}
