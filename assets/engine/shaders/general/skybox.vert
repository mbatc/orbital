#version 430

// TODO: Handle Includes
// #include "../common.h.glsl"

// common.h.glsl
// -----------------------------

#define LightType_Sun   0
#define LightType_Point 1
#define LightType_Spot  2

struct LightBuffer {
  vec3 position;
  int  type;

  vec3 colour;
  float padding0;

  vec3 ambient;
  float strength;

  vec3  attenuation;
  float innerCutoff;

  vec3  direction;
  float outerCutoff;
};

layout(std140, binding=0) uniform Camera {
  mat4 viewProjMatrix;
  mat4 viewMatrix;
  mat4 projMatrix;
  mat4 invViewProjMatrix;
  mat4 invViewMatrix;
  mat4 invProjMatrix;
};

layout(std140, binding=1) uniform Model {
  mat4 modelMatrix;
  mat4 normalMatrix;
  mat4 mvpMatrix;
};

layout(std140, binding=2) buffer lights {
  LightBuffer lightData[];
};

// -----------------------------

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

out vec3 vsout_direction0;

layout(binding = 0, std140) uniform Camera
{
  mat4 viewMat;
  mat4 projMat;
  mat4 camMat;
  mat4 invProjMat;
};

void main()
{
  gl_Position = vec4(position0, 1);

  vsout_uv0 = uvs[gl_VertexID];
  vsout_position0 = vec3(positions[gl_VertexID], 0);
  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);

  // Calculate world space direction
  mat3 rotation = mat3(camMat);
  vec4 direction = invProjMat * vec4(position0, 1);
  direction /= direction.w;
  vsout_direction0 = normalize(rotation * vec3(direction));
}
