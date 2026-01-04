#ifndef PROCEDURAL_TERRAIN_COMMON_GLSL
#define PROCEDURAL_TERRAIN_COMMON_GLSL

#include "../noise.glsl"

// TODO: These could be defined by the application compiling
//       the shader.
#define BND_UBO_Terrain 4

layout(std140, binding=BND_UBO_Terrain) uniform Terrain {
  vec2  sampleOffset;
  uint  seed;
  float maxHeight;
  float minHeight;
  float scale;
} terrain;

#endif // PROCEDURAL_TERRAIN_COMMON_GLSL
