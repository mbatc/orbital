#ifndef GBUFFER_GLSL
#define GBUFFER_GLSL

// TODO: These could be defined by the application compiling
//       the shader.
#define BND_TEX_G_BaseColour  8
#define BND_TEX_G_Ambient     9
#define BND_TEX_G_Position    10
#define BND_TEX_G_Normal      11
#define BND_TEX_G_RMA         12

layout(binding = BND_TEX_G_BaseColour) uniform sampler2D G_BaseColour;
layout(binding = BND_TEX_G_Ambient)    uniform sampler2D G_Ambient;
layout(binding = BND_TEX_G_Position)   uniform sampler2D G_Position;
layout(binding = BND_TEX_G_Normal)     uniform sampler2D G_Normal;
layout(binding = BND_TEX_G_RMA)        uniform sampler2D G_RMA;

vec4 gbuffer_ReadColour(vec2 uv)
{
  return texture2D(G_BaseColour, uv);
}

vec3 gbuffer_ReadPosition(vec2 uv)
{
  return texture2D(G_Position, uv).xyz;
}

vec3 gbuffer_ReadAmbient(vec2 uv)
{
  return texture2D(G_Ambient, uv).xyz;
}

vec3 gbuffer_ReadRMA(vec2 uv)
{
  return texture2D(G_RMA, uv).xyz;
}

vec3 gbuffer_ReadNormal(vec2 uv)
{
  return 2 * (texture2D(G_Normal, uv).xyz - vec3(0.5));
}

#endif // GBUFFER_GLSL
