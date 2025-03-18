#version 430

#include "../common.h.glsl"

const vec2 positions[4] = vec2[] (
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

const int indices[6] = int[] (
    0, 1, 2,
    0, 2, 3
);

out vec3 vsout_direction0;

void main()
{
  gl_Position = vec4(positions[indices[gl_VertexID]], 0, 1);

  // Calculate world space direction
  mat3 rotation = mat3(invViewMatrix);
  vec4 direction = invProjMatrix * vec4(positions[indices[gl_VertexID]], 0, 1);
  direction /= direction.w;
  vsout_direction0 = normalize(rotation * vec3(direction));
}
