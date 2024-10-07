#version 430

layout(binding=0) uniform sampler2D sceneTex;

uniform float threshold;

in vec2 vsout_uv0;

layout(location=0) out vec3 filtered;

void main()
{
  vec3 colour = texture(sceneTex, vsout_uv0).rgb;
  float brightness = max(colour.r, max(colour.g, colour.b));
  float contribution = max(0, brightness - threshold);
  contribution /= max(brightness, 0.00001);
  filtered = colour * contribution;
}
