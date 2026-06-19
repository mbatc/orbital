#version 430

layout(binding=0) uniform sampler2D transmittance;

in vec2 vsout_uv0;

out vec4 fragColour;

void main() {
  fragColour = texture2D(transmittance, vsout_uv0);
}
