#pragma once

#include "util/types.hpp"
#include <vector>
#include <array>

namespace mcserver {

// Perlin Noise implementation for terrain generation
class PerlinNoise {
public:
    explicit PerlinNoise(i64 seed = 0);

    // Get noise value at a 2D point (range: approximately -1.0 to 1.0)
    f64 noise_2d(f64 x, f64 y) const;

    // Octave noise - multiple layers of noise for more natural terrain
    // octaves: number of noise layers to combine
    // persistence: amplitude multiplier for each octave (typically 0.5)
    f64 octave_noise_2d(f64 x, f64 y, i32 octaves, f64 persistence) const;

    // Get noise value normalized to 0.0 to 1.0 range
    f64 noise_2d_01(f64 x, f64 y) const;
    f64 octave_noise_2d_01(f64 x, f64 y, i32 octaves, f64 persistence) const;

private:
    std::array<i32, 512> permutation_;

    // Fade function for smooth interpolation
    static f64 fade(f64 t);

    // Linear interpolation
    static f64 lerp(f64 t, f64 a, f64 b);

    // Gradient calculation
    static f64 grad(i32 hash, f64 x, f64 y);
};

} // namespace mcserver
