#version 430

layout(binding=0) uniform sampler2D sceneTex;
layout(binding=1) uniform sampler2D bloomTex;
layout(binding=2) uniform sampler2D dirtTex;

uniform float strength;
uniform float dirtIntensity;

in vec2 vsout_uv0;

layout(location=0) out vec3 mixed;

void main()
{
  vec3 scene = texture(sceneTex, vsout_uv0).rgb;
  vec3 bloom = texture(bloomTex, vsout_uv0).rgb;
  vec3 dirt  = texture(dirtTex, vec2(vsout_uv0.x, 1.0f - vsout_uv0.y)).rgb * dirtIntensity;
  mixed = mix(scene, bloom + bloom * dirt, strength);
}
