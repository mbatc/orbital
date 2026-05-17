#ifndef GBUFFER_GLSL
#define GBUFFER_GLSL

// TODO: These could be defined by the application compiling
//       the shader.
#define BND_TEX_G_BaseColour  0
#define BND_TEX_G_Ambient     1
#define BND_TEX_G_Position    2
#define BND_TEX_G_Normal      3
#define BND_TEX_G_RMA         4

layout(binding = BND_TEX_G_BaseColour) uniform sampler2D G_BaseColour;
layout(binding = BND_TEX_G_Ambient)    uniform sampler2D G_Ambient;
layout(binding = BND_TEX_G_Position)   uniform sampler2D G_Position;
layout(binding = BND_TEX_G_Normal)     uniform sampler2D G_Normal;
layout(binding = BND_TEX_G_RMA)        uniform sampler2D G_RMA;

#endif // GBUFFER_GLSL
