cmake_minimum_required(VERSION 3.23)

project(HelloWorldPlugin VERSION 1.0.0 LANGUAGES CXX)

# C++23 required
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build as shared library (plugin)
add_library(hello_world SHARED
    hello_world_plugin.cpp
)

# Include server headers
target_include_directories(hello_world PRIVATE
    ${CMAKE_SOURCE_DIR}/../../../src
)

# Link with plugin interface
# Note: In a real setup, you'd link against a plugin SDK library
# For now, we just need headers (header-only interfaces)

# Set output directory
set_target_properties(hello_world PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/../../../plugins"
    PREFIX ""  # Remove "lib" prefix on Unix
)

# Platform-specific settings
if(UNIX)
    target_compile_options(hello_world PRIVATE
        -Wall -Wextra -Wpedantic
        -fvisibility=hidden
    )

    # Export only required symbols
    target_link_options(hello_world PRIVATE
        -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/exports.map
    )
endif()

if(WIN32)
    target_compile_options(hello_world PRIVATE
        /W4
    )
endif()
