add_library(storage STATIC
    nbt/nbt.cpp
    nbt/nbt.hpp
    nbt/nbt_io.cpp
    nbt/nbt_io.hpp
    region/region_file.cpp
    region/region_file.hpp
    chunk/chunk_serializer.cpp
    chunk/chunk_serializer.hpp
    chunk/chunk_storage.cpp
    chunk/chunk_storage.hpp
    async/async_io.cpp
    async/async_io.hpp
    player/player_data_manager.cpp
    player/player_data_manager.hpp
)

target_include_directories(storage PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# Find zlib for NBT compression
find_package(ZLIB REQUIRED)

target_link_libraries(storage PUBLIC core util platform world entity ZLIB::ZLIB)

set_target_properties(storage PROPERTIES FOLDER "Storage")
