#ifndef HASH_GLSL
#define HASH_GLSL

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

#endif // HASH_GLSL
