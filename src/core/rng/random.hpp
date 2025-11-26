#pragma once

#include "util/types.hpp"
#include <random>

namespace mcserver {

// Java-compatible random number generator for world generation parity
class Random {
public:
    explicit Random(i64 seed = 0);

    void set_seed(i64 seed);
    i64 get_seed() const { return seed_; }

    // Generate random integer
    i32 next_int();
    i32 next_int(i32 bound);

    // Generate random long
    i64 next_long();

    // Generate random float [0, 1)
    f32 next_float();

    // Generate random double [0, 1)
    f64 next_double();

    // Generate random boolean
    bool next_bool();

private:
    i64 seed_;
    i64 next_seed();
};

} // namespace mcserver
