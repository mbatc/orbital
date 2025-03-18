#ifndef GBUFFER_GLSL
#define GBUFFER_GLSL

layout(binding = 0) uniform sampler2D G_BaseColour;
layout(binding = 1) uniform sampler2D G_Ambient;
layout(binding = 2) uniform sampler2D G_Position;
layout(binding = 3) uniform sampler2D G_Normal;
layout(binding = 4) uniform sampler2D G_RMA;

#endif // GBUFFER_GLSL
