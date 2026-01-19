#ifndef PROCEDURAL_TERRAIN_COMMON_GLSL
#define PROCEDURAL_TERRAIN_COMMON_GLSL

#include "../noise.glsl"
#include "../common.glsl"

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

struct TerrainBiome {
  vec4  colour;
  vec2  range;
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
    { 0, 0.7 },
    1.0,
    3,
    0.5,
    2,
    0,
    0.6
  },
  {
    { 0.6, 0.6, 0.6, 1 },
    { 0.5, 0.7 },
    1.0,
    3,
    0.5,
    2,
    0.2,
    0.9
  },
  {
    { 1, 1, 1, 1 },
    { 0.6, 1 },
    2.0,
    10,
    0.5,
    2,
    0.5,
    1
  }
};

float sampleBiome(vec2 uv) {
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

float sampleBiomeHeight(vec2 uv, uint biomeIndex) {
  TerrainBiome biome = terrainBiomes[biomeIndex];

  const float noise = perlinNoise(
    abs((terrain.sampleOffset + uv * terrain.sampleSize) * terrain.scale),
    biome.frequency,
    biome.octaves,
    biome.persistance,
    biome.lacurnarity,
    terrain.seed); // multiple octaves

  return mapLinear(noise, -1, 1, biome.minHeight, biome.maxHeight);
}

TerrainSample sampleTerrainByBiome(vec2 uv) {
  TerrainSample ret;
  ret.biome = sampleBiome(uv);

  float totalWeight = 0;
  float totalHeight = 0;

  for (uint i = 0; i < terrainBiomes.length; ++i) {
    TerrainBiome biome = terrainBiomes[i];
    if (ret.biome > biome.range.x && ret.biome <= biome.range.y) {
      const float weight = min(ret.biome - biome.range.x, biome.range.y - ret.biome);
      totalWeight += weight;
      totalHeight += sampleBiomeHeight(uv, i) * weight;
    }
  }

  if (totalWeight != 0) {
    // ret.biomeIndex = uint(min(ret.biome * terrainBiomes.length, terrainBiomes.length - 1));
    ret.height = totalHeight / totalWeight;
  } else {
    ret.height = 0;
  }
  return ret;
}


TerrainSample sampleTerrainHeightByBiome(vec2 uv) {
  TerrainSample ret;
  ret.biome = sampleBiome(uv);

  float totalWeight = 0;
  float totalHeight = 0;

  for (uint i = 0; i < terrainBiomes.length; ++i) {
    TerrainBiome biome = terrainBiomes[i];
    if (ret.biome > biome.range.x && ret.biome <= biome.range.y) {
      const float weight = min(ret.biome - biome.range.x, biome.range.y - ret.biome);
      totalWeight += weight;
      totalHeight += sampleBiomeHeight(uv, i) * weight;
    }
  }

  if (totalWeight != 0) {
    // ret.biomeIndex = uint(min(ret.biome * terrainBiomes.length, terrainBiomes.length - 1));
    ret.height = totalHeight / totalWeight;
  } else {
    ret.height = 0;
  }

  ret.height = mapLinear(ret.height, 0, 1, terrain.minHeight, terrain.maxHeight);

  return ret;
}


float sampleTerrainHeight(vec2 uv) {
  float noise = sampleTerrain(uv);

  return terrain.minHeight + noise * (terrain.maxHeight - terrain.minHeight);
}


#endif // PROCEDURAL_TERRAIN_COMMON_GLSL
