#ifndef POSTPROCESSINPUT_GLSL
#define POSTPROCESSINPUT_GLSL

// TODO: These could be defined by the application compiling
//       the shader.
#define BND_TEX_SceneColour 0
#define BND_TEX_SceneDepth  1
#define BND_TEX_BaseColour  2
#define BND_TEX_Ambient     3
#define BND_TEX_Position    4
#define BND_TEX_Normal      5
#define BND_TEX_RMA         6

layout(binding = BND_TEX_SceneColour) uniform sampler2D sceneColourTex;
layout(binding = BND_TEX_SceneDepth)  uniform sampler2D sceneDepthTex;
layout(binding = BND_TEX_BaseColour)  uniform sampler2D baseColourTex;
layout(binding = BND_TEX_Ambient)     uniform sampler2D ambientTex;
layout(binding = BND_TEX_Position)    uniform sampler2D positionTex;
layout(binding = BND_TEX_Normal)      uniform sampler2D normalTex;
layout(binding = BND_TEX_RMA)         uniform sampler2D RMATex;


vec4 pps_ReadSceneColour(vec2 uv)
{
  return texture2D(sceneColourTex, uv);
}

vec3 pps_ReadBaseColour(vec2 uv)
{
  return texture2D(baseColourTex, uv).xyz;
}

float pps_ReadDepth(vec2 uv)
{
  return texture2D(sceneDepthTex, uv).x;
}

vec3 pps_ReadAmbientColour(vec2 uv)
{
  return texture2D(ambientTex, uv).xyz;
}

vec3 pps_ReadPosition(vec2 uv)
{
  return texture2D(positionTex, uv).xyz;
}

vec3 pps_ReadNormal(vec2 uv)
{
  return normalize(2 * (texture2D(normalTex, uv).xyz - vec3(0.5)));
}

vec3 pps_ReadRMA(vec2 uv)
{
  return texture2D(RMATex, uv).xyz;
}

#endif // POSTPROCESSINPUT_GLSL
