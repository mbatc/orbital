#version 430

#include "../common.h.glsl"

in layout(location = 0) vec3 position0;
in layout(location = 2) vec2 uv0;
in layout(location = 3) vec3 normal0;
in layout(location = 4) vec3 tangent0;

void main()
{
  gl_Position = mvpMatrix * vec4(position0, 1);
}
