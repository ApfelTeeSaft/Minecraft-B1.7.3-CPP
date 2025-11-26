#include "util/allocator/arena.hpp"
#include <iostream>
#include <cassert>

using namespace mcserver;

struct TestObject {
    int value;
    TestObject(int v) : value(v) {}
};

int test_arena() {
    std::cout << "Testing arena allocator...\n";

    Arena arena(1024);

    // Test basic allocation
    {
        void* ptr1 = arena.allocate(100);
        assert(ptr1 != nullptr);

        void* ptr2 = arena.allocate(200);
        assert(ptr2 != nullptr);
        assert(ptr2 != ptr1);

        std::cout << "  ✓ Basic allocation\n";
    }

    // Test object creation
    {
        TestObject* obj = arena.create<TestObject>(42);
        assert(obj != nullptr);
        assert(obj->value == 42);

        std::cout << "  ✓ Object creation\n";
    }

    // Test reset
    {
        arena.reset();
        assert(arena.used() == 0);

        std::cout << "  ✓ Arena reset\n";
    }

    // Test alignment
    {
        void* ptr1 = arena.allocate(1, 16);
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr1);
        assert(addr % 16 == 0);

        std::cout << "  ✓ Aligned allocation\n";
    }

    return 0;
}
