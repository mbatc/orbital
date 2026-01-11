#ifndef PROCEDURAL_TERRAIN_COMMON_GLSL
#define PROCEDURAL_TERRAIN_COMMON_GLSL

#include "../noise.glsl"

// TODO: These could be defined by the application compiling
//       the shader.
#define BND_UBO_Terrain 4

layout(std140, binding=BND_UBO_Terrain) uniform Terrain {
  vec2  sampleOffset;
  float sampleSize;
  uint  seed;
  float minHeight;
  float maxHeight;
  float scale;
  float frequency;
  uint  octaves;
  float persistance;
  float lacurnarity;
} terrain;

struct TerrainBiome {
  vec4 colour;
  float frequency;
  uint  octaves;
  float persistance;
  float lacurnarity;
  float minHeight;
  float maxHeight;
};

TerrainBiome terrainBiomes[3] = {
  {
    { 0.3, 1, 0.3, 1 },
    1.0,
    3,
    0.5,
    2,
    0,
    0.52
  },
  {
    { 0.6, 0.6, 0.6, 1 },
    1.0,
    3,
    0.5,
    2,
    0,
    0.52
  },
  {
    { 1, 1, 1, 1 },
    1.0,
    3,
    0.5,
    2,
    0,
    0.52
  }
};

float sampleBiome(vec2 uv) {
  float noise = perlinNoise(
    abs((terrain.sampleOffset + uv * terrain.sampleSize) * terrain.scale),
    terrain.frequency,
    1,
    terrain.persistance,
    terrain.lacurnarity,
    terrain.seed); // multiple octaves
  return (noise + 1.0) * 0.5; // convert from range [-1, 1] to range [0, 1]
}

float sampleTerrain(vec2 uv) {
  float noise = perlinNoise(
    abs((terrain.sampleOffset + uv * terrain.sampleSize) * terrain.scale),
    terrain.frequency,
    terrain.octaves,
    terrain.persistance,
    terrain.lacurnarity,
    terrain.seed); // multiple octaves
  return (noise + 1.0) * 0.5; // convert from range [-1, 1] to range [0, 1]
}

struct TerrainSample {
  float height;
  uint  biomeIndex;
  float biome;
};

float sampleBiome(vec2 uv) {

}

float sampleTerrainByBiome(vec2 uv) {
  TerrainSample sample;
  sample.biome      = sampleBiome(uv);
  sample.biomeIndex = min(biome * terrainBiomes.length, terrainBiomes.length - 1);
  sample.height     = sampleTerrain();


  return sample;
}

float sampleTerrainHeight(vec2 uv) {
  float noise = sampleTerrain(uv);

  return terrain.minHeight + noise * (terrain.maxHeight - terrain.minHeight);
}


#endif // PROCEDURAL_TERRAIN_COMMON_GLSL
