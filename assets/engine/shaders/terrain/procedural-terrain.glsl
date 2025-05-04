#ifndef PROCEDURAL_TERRAIN_GLSL
#define PROCEDURAL_TERRAIN_GLSL

struct ProceduralTerrain
{
  uint seed;
};

/// Calculate the terrain height at a specific coordinate
float ProceduralTerrain_calculateHeight(ProceduralTerrain terrain, vec3 coord)
{

}

/// Get the biome classification for a terrain coordinate
float ProceduralTerrain_calculateBiome(vec3 coordinate, float averageTemperature, float )
{
  
}

#endif // PROCEDURAL_TERRAIN_GLSL
