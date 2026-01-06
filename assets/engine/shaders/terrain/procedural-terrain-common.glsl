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

float sampleTerrain(vec2 uv) {
  float noise = perlinNoise((terrain.sampleOffset + uv) * terrain.scale, 1, 6, 0.5, 2.0, terrain.seed); // multiple octaves
  return (noise + 1.0) * 0.5; // convert from range [-1, 1] to range [0, 1]
}

float sampleTerrainHeight(vec2 uv) {
  float noise = sampleTerrain(uv);

  return terrain.minHeight + noise * (terrain.maxHeight - terrain.minHeight);
}


#endif // PROCEDURAL_TERRAIN_COMMON_GLSL
