#version 430

const vec2 positions[3] = vec2[] (
    vec2(-1.0, -1.0),
    vec2( 3.0, -1.0),
    vec2(-1.0,  3.0)
);

const vec2 uvs[3] = vec2[] (
    vec2(0.0, 0.0),
    vec2(2.0, 0.0),
    vec2(0.0, 2.0)
);

out vec2 vsout_uv0;
out vec3 vsout_position0;

void main() {
  vsout_uv0 = uvs[gl_VertexID];
  vsout_position0 = vec3(positions[gl_VertexID], 0);
  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}
