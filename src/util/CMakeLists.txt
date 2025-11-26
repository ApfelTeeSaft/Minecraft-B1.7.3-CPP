add_library(util STATIC
    log/logger.cpp
    log/logger.hpp
    allocator/arena.cpp
    allocator/arena.hpp
    allocator/pool.cpp
    allocator/pool.hpp
    uuid.cpp
    uuid.hpp
    span_util.hpp
    result.hpp
    types.hpp
)

target_include_directories(util PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(util PUBLIC platform)

set_target_properties(util PROPERTIES FOLDER "Core")
