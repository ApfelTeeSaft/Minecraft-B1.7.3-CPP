#include <iostream>


int test_packet();
int test_arena();
int test_random();

int main() {
    std::cout << "Running unit tests...\n";

    int failed = 0;

    failed += test_packet();
    failed += test_arena();
    failed += test_random();

    if (failed == 0) {
        std::cout << "\nAll tests passed!\n";
        return 0;
    } else {
        std::cout << "\n" << failed << " test(s) failed!\n";
        return 1;
    }
}
