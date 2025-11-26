add_library(world STATIC
    chunk/chunk.cpp
    chunk/chunk.hpp
    chunk/chunk_manager.cpp
    chunk/chunk_manager.hpp
    generation/world_generator.cpp
    generation/world_generator.hpp
    generation/noise.cpp
    generation/noise.hpp
    lighting/lighting_engine.cpp
    lighting/lighting_engine.hpp
    region/region.cpp
    region/region.hpp
    block/block_manager.cpp
    block/block_manager.hpp
)

target_include_directories(world PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(world PUBLIC core util platform)

set_target_properties(world PROPERTIES FOLDER "World")
