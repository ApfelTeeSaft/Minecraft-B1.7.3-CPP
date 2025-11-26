#include "noise.hpp"
#include "core/rng/random.hpp"
#include <cmath>
#include <algorithm>

namespace mcserver {

PerlinNoise::PerlinNoise(i64 seed) {
    // Initialize permutation table with values 0-255
    std::array<i32, 256> p;
    for (i32 i = 0; i < 256; ++i) {
        p[i] = i;
    }

    // Shuffle using seed
    Random rng(seed);
    for (i32 i = 255; i > 0; --i) {
        i32 j = rng.next_int(i + 1);
        std::swap(p[i], p[j]);
    }

    // Duplicate the permutation table to avoid overflow
    for (i32 i = 0; i < 256; ++i) {
        permutation_[i] = p[i];
        permutation_[256 + i] = p[i];
    }
}

f64 PerlinNoise::noise_2d(f64 x, f64 y) const {
    // Find unit grid cell containing point
    i32 X = static_cast<i32>(std::floor(x)) & 255;
    i32 Y = static_cast<i32>(std::floor(y)) & 255;

    // Get relative position within cell
    x -= std::floor(x);
    y -= std::floor(y);

    // Compute fade curves
    f64 u = fade(x);
    f64 v = fade(y);

    // Hash coordinates of the 4 cube corners
    i32 aa = permutation_[permutation_[X] + Y];
    i32 ab = permutation_[permutation_[X] + Y + 1];
    i32 ba = permutation_[permutation_[X + 1] + Y];
    i32 bb = permutation_[permutation_[X + 1] + Y + 1];

    // Blend results from 4 corners
    f64 res = lerp(v,
        lerp(u, grad(aa, x, y), grad(ba, x - 1, y)),
        lerp(u, grad(ab, x, y - 1), grad(bb, x - 1, y - 1))
    );

    return res;
}

f64 PerlinNoise::octave_noise_2d(f64 x, f64 y, i32 octaves, f64 persistence) const {
    f64 total = 0.0;
    f64 frequency = 1.0;
    f64 amplitude = 1.0;
    f64 max_value = 0.0;

    for (i32 i = 0; i < octaves; ++i) {
        total += noise_2d(x * frequency, y * frequency) * amplitude;

        max_value += amplitude;
        amplitude *= persistence;
        frequency *= 2.0;
    }

    return total / max_value;
}

f64 PerlinNoise::noise_2d_01(f64 x, f64 y) const {
    return (noise_2d(x, y) + 1.0) * 0.5;
}

f64 PerlinNoise::octave_noise_2d_01(f64 x, f64 y, i32 octaves, f64 persistence) const {
    return (octave_noise_2d(x, y, octaves, persistence) + 1.0) * 0.5;
}

f64 PerlinNoise::fade(f64 t) {
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

f64 PerlinNoise::lerp(f64 t, f64 a, f64 b) {
    return a + t * (b - a);
}

f64 PerlinNoise::grad(i32 hash, f64 x, f64 y) {
    // Convert low 4 bits of hash into 8 gradient directions
    i32 h = hash & 7;
    f64 u = h < 4 ? x : y;
    f64 v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

} // namespace mcserver
