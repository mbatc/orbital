#ifndef PROCEDURAL_TERRAIN_INPUTS_GLSL
#define PROCEDURAL_TERRAIN_INPUTS_GLSL

// TODO: These could be defined by the application compiling
//       the shader.
#define BND_UBO_Material 3

#define BND_TEX_BaseColourMap  0
#define BND_TEX_AmbientMap     1
#define BND_TEX_EmissiveMap    2
#define BND_TEX_RoughnessMap   3
#define BND_TEX_MetalnessMap   4
#define BND_TEX_AoMap          5
#define BND_TEX_AlphaMap       6
#define BND_TEX_NormalMap      7

layout(std140, binding=BND_UBO_Material) uniform ProceduralTerrain {
  int   seed        = 0;
  int   frequency   = 1;
  int   octaves     = 6;
  float persistence = 0.5;
  float lacunarity  = 2.0;
  float padding[3];
};

layout(binding = BND_TEX_BaseColourMap) uniform sampler2D baseColourMap;
layout(binding = BND_TEX_AmbientMap)    uniform sampler2D ambientMap;
layout(binding = BND_TEX_EmissiveMap)   uniform sampler2D emissiveMap;
layout(binding = BND_TEX_RoughnessMap)  uniform sampler2D roughnessMap;
layout(binding = BND_TEX_MetalnessMap)  uniform sampler2D metalnessMap;
layout(binding = BND_TEX_AoMap)         uniform sampler2D aoMap;
layout(binding = BND_TEX_AlphaMap)      uniform sampler2D alphaMap;
layout(binding = BND_TEX_NormalMap)     uniform sampler2D normalMap;

#endif // PROCEDURAL_TERRAIN_INPUTS_GLSL
