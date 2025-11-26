#include "random.hpp"

namespace mcserver {

// Java LCG constants for compatibility
static constexpr i64 MULTIPLIER = 0x5DEECE66DLL;
static constexpr i64 ADDEND = 0xBLL;
static constexpr i64 MASK = (1LL << 48) - 1;

Random::Random(i64 seed) {
    set_seed(seed);
}

void Random::set_seed(i64 seed) {
    seed_ = (seed ^ MULTIPLIER) & MASK;
}

i64 Random::next_seed() {
    seed_ = (seed_ * MULTIPLIER + ADDEND) & MASK;
    return seed_;
}

i32 Random::next_int() {
    return static_cast<i32>(next_seed() >> 16);
}

i32 Random::next_int(i32 bound) {
    if (bound <= 0) {
        return 0;
    }

    if ((bound & -bound) == bound) {
        // Power of 2
        return static_cast<i32>((bound * static_cast<i64>(next_seed() >> 16)) >> 31);
    }

    i32 bits, val;
    do {
        bits = static_cast<i32>(next_seed() >> 16);
        val = bits % bound;
    } while (bits - val + (bound - 1) < 0);

    return val;
}

i64 Random::next_long() {
    return (static_cast<i64>(next_int()) << 32) + next_int();
}

f32 Random::next_float() {
    return static_cast<f32>(next_seed() >> 16) / static_cast<f32>(1 << 24);
}

f64 Random::next_double() {
    return static_cast<f64>((static_cast<i64>(next_seed() >> 16) << 27) + (next_seed() >> 16)) /
           static_cast<f64>(1LL << 53);
}

bool Random::next_bool() {
    return next_int() >= 0;
}

} // namespace mcserver
