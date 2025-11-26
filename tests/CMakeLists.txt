add_executable(tests_unit
    unit/test_main.cpp
    unit/test_packet.cpp
    unit/test_arena.cpp
    unit/test_random.cpp
)

target_link_libraries(tests_unit PRIVATE
    platform
    util
    core
    net
    world
    entity
    storage
)

add_test(NAME unit_tests COMMAND tests_unit)

set_target_properties(tests_unit PROPERTIES FOLDER "Tests")
