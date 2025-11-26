#include "core/rng/random.hpp"
#include <iostream>
#include <cassert>

using namespace mcserver;

int test_random() {
    std::cout << "Testing Java-compatible RNG...\n";

    // Test deterministic behavior (same seed = same sequence)
    {
        Random rng1(12345);
        Random rng2(12345);

        for (int i = 0; i < 100; ++i) {
            assert(rng1.next_int() == rng2.next_int());
        }

        std::cout << "  ✓ Deterministic sequence\n";
    }

    // Test bounds
    {
        Random rng(54321);

        for (int i = 0; i < 1000; ++i) {
            int val = rng.next_int(100);
            assert(val >= 0 && val < 100);
        }

        std::cout << "  ✓ Bounded random\n";
    }

    // Test float range
    {
        Random rng(99999);

        for (int i = 0; i < 1000; ++i) {
            float val = rng.next_float();
            assert(val >= 0.0f && val < 1.0f);
        }

        std::cout << "  ✓ Float range\n";
    }

    // Test seed consistency
    {
        Random rng(42);
        int first = rng.next_int();

        rng.set_seed(42);
        int second = rng.next_int();

        assert(first == second);

        std::cout << "  ✓ Seed reset\n";
    }

    return 0;
}
