#version 430

// TODO: Handle Includes
// #include "common.h.glsl"

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
  
  uvec2 shadowMapRange;
  float padding1;
  float padding2;
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
