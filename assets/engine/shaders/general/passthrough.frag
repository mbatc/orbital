#version 430

layout(binding=0) uniform sampler2D colourTexture;

in vec2 vsout_uv0;

out vec4 fragColour;

void main() {
  fragColour = texture2D(colourTexture, vsout_uv0);
}
