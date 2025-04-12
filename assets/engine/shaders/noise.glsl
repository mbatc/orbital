#ifndef NOISE_GLSL
#define NOISE_GLSL

// implementation of MurmurHash (https://sites.google.com/site/murmurhash/) for a 
// single unsigned integer.
uint hash(uint x, uint seed) {
  const uint m = 0x5bd1e995U;
  uint hash = seed;
  // process input
  uint k = x;
  k *= m;
  k ^= k >> 24;
  k *= m;
  hash *= m;
  hash ^= k;
  // some final mixing
  hash ^= hash >> 13;
  hash *= m;
  hash ^= hash >> 15;
  return hash;
}

uint hash(uvec2 v, uint seed) {
  seed = hash(v.x, seed);
  return hash(v.y, seed);
}

uint hash(uvec3 v, uint seed) {
  seed = hash(v.x, seed);
  seed = hash(v.y, seed);
  return hash(v.z, seed);
}

vec3 perlinGradientsTable[16] = vec3[](
  vec3(1, 1, 0),
  vec3(-1, 1, 0),
  vec3(1, -1, 0),
  vec3(-1, -1, 0),
  vec3(1, 0, 1),
  vec3(-1, 0, 1),
  vec3(1, 0, -1),
  vec3(-1, 0, -1),
  vec3(0, 1, 1),
  vec3(0, -1, 1),
  vec3(0, 1, -1),
  vec3(0, -1, -1),
  vec3(1, 1, 0),
  vec3(-1, 1, 0),
  vec3(0, -1, 1),
  vec3(0, -1, -1)
);

vec3 perlinGradientDirection3(uint hash) {
  return perlinGradientsTable[int(hash) & 15];
}

vec2 perlinGradientDirection2(uint hash) {
  return perlinGradientsTable[int(hash) & 3].xy;
}

vec2 perlinGradientDirection(uvec2 coord, uint seed) {
  return perlinGradientDirection2(hash(coord, seed));
}

vec3 perlinGradientDirection(uvec3 coord, uint seed) {
  return perlinGradientDirection3(hash(coord, seed));
}

float perlinInterpolate(float value1, float value2, float value3, float value4, vec2 t) {
    return mix(mix(value1, value2, t.x), mix(value3, value4, t.x), t.y);
}

float perlinInterpolate(float value1, float value2, float value3, float value4, float value5, float value6, float value7, float value8, vec3 t) {
    return mix(
        mix(mix(value1, value2, t.x), mix(value3, value4, t.x), t.y),
        mix(mix(value5, value6, t.x), mix(value7, value8, t.x), t.y),
        t.z
    );
}

vec2 perlinFade(vec2 t) {
  // 6t^5 - 15t^4 + 10t^3
	return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

vec3 perlinFade(vec3 t) {
  // 6t^5 - 15t^4 + 10t^3
	return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

float perlinNoise(vec2 position, uint seed) {
    vec2 floorPosition = floor(position);
    vec2 fractPosition = position - floorPosition;
    uvec2 cellCoordinates = uvec2(floorPosition);
    float value1 = dot(perlinGradientDirection(cellCoordinates, seed), fractPosition);
    float value2 = dot(perlinGradientDirection(cellCoordinates + uvec2(1, 0), seed), fractPosition - vec2(1.0, 0.0));
    float value3 = dot(perlinGradientDirection(cellCoordinates + uvec2(0, 1), seed), fractPosition - vec2(0.0, 1.0));
    float value4 = dot(perlinGradientDirection(cellCoordinates + uvec2(1, 1), seed), fractPosition - vec2(1.0, 1.0));
    return perlinInterpolate(value1, value2, value3, value4, perlinFade(fractPosition));
}

float perlinNoise(vec2 position, int frequency, int octaveCount, float persistence, float lacunarity, uint seed) {
    float value = 0.0;
    float amplitude = 1.0;
    float currentFrequency = float(frequency);
    uint currentSeed = seed;
    for (int i = 0; i < octaveCount; i++) {
        currentSeed = hash(currentSeed, 0x0U); // create a new seed for each octave
        value += perlinNoise(position * currentFrequency, currentSeed) * amplitude;
        amplitude *= persistence;
        currentFrequency *= lacunarity;
    }
    return value;
}

float perlinNoise(vec3 position, uint seed) {
    vec3 floorPosition = floor(position);
    vec3 fractPosition = position - floorPosition;
    uvec3 cellCoordinates = uvec3(floorPosition);
    float value1 = dot(perlinGradientDirection(cellCoordinates, seed), fractPosition);
    float value2 = dot(perlinGradientDirection(cellCoordinates + uvec3(1, 0, 0), seed), fractPosition - vec3(1, 0, 0));
    float value3 = dot(perlinGradientDirection(cellCoordinates + uvec3(0, 1, 0), seed), fractPosition - vec3(0, 1, 0));
    float value4 = dot(perlinGradientDirection(cellCoordinates + uvec3(1, 1, 0), seed), fractPosition - vec3(1, 1, 0));
    float value5 = dot(perlinGradientDirection(cellCoordinates + uvec3(0, 0, 1), seed), fractPosition - vec3(0, 0, 1));
    float value6 = dot(perlinGradientDirection(cellCoordinates + uvec3(1, 0, 1), seed), fractPosition - vec3(1, 0, 1));
    float value7 = dot(perlinGradientDirection(cellCoordinates + uvec3(0, 1, 1), seed), fractPosition - vec3(0, 1, 1));
    float value8 = dot(perlinGradientDirection(cellCoordinates + uvec3(1, 1, 1), seed), fractPosition - vec3(1, 1, 1));
    return perlinInterpolate(value1, value2, value3, value4, value5, value6, value7, value8, perlinFade(fractPosition));
}

float perlinNoise(vec3 position, int frequency, int octaveCount, float persistence, float lacunarity, uint seed) {
    float value = 0.0;
    float amplitude = 1.0;
    float currentFrequency = float(frequency);
    uint currentSeed = seed;
    for (int i = 0; i < octaveCount; i++) {
        currentSeed = hash(currentSeed, 0x0U); // create a new seed for each octave
        value += perlinNoise(position * currentFrequency, currentSeed) * amplitude;
        amplitude *= persistence;
        currentFrequency *= lacunarity;
    }
    return value;
}

#endif // NOISE_GLSL