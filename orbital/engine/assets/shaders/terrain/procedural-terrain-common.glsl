#ifndef PROCEDURAL_TERRAIN_COMMON_GLSL
#define PROCEDURAL_TERRAIN_COMMON_GLSL

#include "../noise.glsl"

// TODO: These could be defined by the application compiling
//       the shader.
#define BND_UBO_Terrain     4
#define BND_UBO_TerrainTile 5

layout(std140, binding=BND_UBO_Terrain) uniform Terrain {
  uint  seed;
  float minHeight;
  float maxHeight;
  float scale;
  float frequency;
  uint  octaves;
  float persistance;
  float lacurnarity;
} terrain;

layout(std140, binding=BND_UBO_TerrainTile) uniform TerrainTile {
  mat4  sampleTransform;
  float tileSize;
} terrainTile;

float sampleTerrain(vec3 coord) {
  float noise = perlinNoise(
    (coord + vec3(2)) * terrain.scale,
    terrain.frequency,
    terrain.octaves,
    terrain.persistance,
    terrain.lacurnarity,
    terrain.seed); // multiple octaves
  return (noise + 1.0) * 0.5; // convert from range [-1, 1] to range [0, 1]
}

float sampleTerrainHeight(vec3 coord) {
  float noise = sampleTerrain(coord);

  return terrain.minHeight + noise * (terrain.maxHeight - terrain.minHeight);
}

#endif // PROCEDURAL_TERRAIN_COMMON_GLSL
