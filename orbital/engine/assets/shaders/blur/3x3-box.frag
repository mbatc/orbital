#version 420

layout(binding = 0) uniform sampler2D texture0;

in vec2 vsout_uv0;

out vec4 fragColour;

void main()
{
  vec2 stepSize = 1 / textureSize(texture0, 0);
  vec4 colour = vec4(0);
  for (int y = -1; y <= 1; ++y) {
    for (int x = -1; x <= 1; ++x) {
      colour += textureLod(texture0, vsout_uv0 + vec2(x, y) * stepSize, 0);
    }
  };

  fragColour = max(colour / 9, 0.0001);
}
