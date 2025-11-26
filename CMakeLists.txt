cmake_minimum_required(VERSION 3.23)

# Auto-detect and configure vcpkg on Windows if available
if(WIN32 AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # Common vcpkg locations
    set(VCPKG_PATHS
        "$ENV{VCPKG_ROOT}"
        "C:/vcpkg"
        "C:/src/vcpkg"
        "$ENV{USERPROFILE}/vcpkg"
    )

    # Also check for vcpkg with date suffix (e.g., vcpkg-2025.06.13)
    file(GLOB VCPKG_DATED "$ENV{USERPROFILE}/vcpkg-*")
    list(APPEND VCPKG_PATHS ${VCPKG_DATED})

    foreach(VCPKG_PATH ${VCPKG_PATHS})
        if(EXISTS "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake")
            set(CMAKE_TOOLCHAIN_FILE "${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
            message(STATUS "Auto-detected vcpkg at: ${VCPKG_PATH}")
            break()
        endif()
    endforeach()

    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        message(STATUS "vcpkg not found. Install packages manually or set VCPKG_ROOT environment variable.")
    endif()
endif()

project(MinecraftBeta173Server
    VERSION 0.1.0
    DESCRIPTION "Modern C++ implementation of Minecraft Beta 1.7.3 Server"
    LANGUAGES CXX
)

# C++23 required, C++20 minimum
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fallback to C++20 if C++23 is not available
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-std=c++23" COMPILER_SUPPORTS_CXX23)
    if(NOT COMPILER_SUPPORTS_CXX23)
        set(CMAKE_CXX_STANDARD 20)
        message(STATUS "C++23 not available, falling back to C++20")
    endif()
endif()

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_TESTS "Build unit tests" ON)
option(USE_ASAN "Enable AddressSanitizer" OFF)
option(USE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(PROFILE_BUILD "Enable profiling" OFF)
option(ALLOW_UNSAFE_PLUGINS "Allow unsafe plugin operations (disable sandbox)" OFF)

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_compile_definitions(PLATFORM_WINDOWS=1)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    add_compile_definitions(PLATFORM_MACOS=1)
elseif(UNIX)
    set(PLATFORM_LINUX TRUE)
    add_compile_definitions(PLATFORM_LINUX=1)
endif()

# Compiler-specific flags
if(MSVC)
    add_compile_options(
        /W4                 # Warning level 4
        /WX                 # Warnings as errors
        /permissive-        # Conformance mode
        /Zc:__cplusplus     # Enable updated __cplusplus macro
        /MP                 # Multi-processor compilation
        /EHsc               # Exception handling
    )
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
else()
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        -fno-exceptions     # Disable exceptions (we use result types)
    )

    if(USE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()

    if(USE_UBSAN)
        add_compile_options(-fsanitize=undefined)
        add_link_options(-fsanitize=undefined)
    endif()

    if(PROFILE_BUILD)
        add_compile_options(-pg)
        add_link_options(-pg)
    endif()
endif()

# Configure unsafe plugins flag
if(ALLOW_UNSAFE_PLUGINS)
    add_compile_definitions(ALLOW_UNSAFE_PLUGINS=1)
    message(WARNING "ALLOW_UNSAFE_PLUGINS is enabled - sandbox checks are disabled!")
endif()

# Find required libraries
find_package(ZLIB REQUIRED)

# Helpful message if ZLIB is not found
if(NOT ZLIB_FOUND AND WIN32)
    message(FATAL_ERROR
        "ZLIB not found!\n"
        "On Windows with vcpkg, install it with:\n"
        "  vcpkg install zlib:x64-windows\n"
        "Or set VCPKG_ROOT environment variable to your vcpkg installation."
    )
endif()

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/sdk)

# Add subdirectories
add_subdirectory(src/platform)
add_subdirectory(src/util)
add_subdirectory(src/core)
add_subdirectory(src/net)
add_subdirectory(src/world)
add_subdirectory(src/entity)
add_subdirectory(src/storage)
add_subdirectory(src/admin)
add_subdirectory(src/plugin)

# Main server executable
add_executable(mcserver
    src/main.cpp
)

target_link_libraries(mcserver PRIVATE
    platform
    util
    core
    net
    world
    entity
    storage
    admin
    plugin
    ZLIB::ZLIB
)

# Platform-specific libraries
if(PLATFORM_WINDOWS)
    target_link_libraries(mcserver PRIVATE ws2_32 mswsock)
elseif(PLATFORM_LINUX)
    target_link_libraries(mcserver PRIVATE pthread)
elseif(PLATFORM_MACOS)
    target_link_libraries(mcserver PRIVATE pthread)
endif()

# Set output directories
set_target_properties(mcserver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/out/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/out/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}/Release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/out/${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}/RelWithDebInfo"
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# IDE support: Group projects in folders for Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Print configuration summary
message(STATUS "")
message(STATUS "=== Minecraft Beta 1.7.3 Server Configuration ===")
message(STATUS "Build type:              ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard:            ${CMAKE_CXX_STANDARD}")
message(STATUS "Platform:                ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler:                ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build tests:             ${BUILD_TESTS}")
message(STATUS "AddressSanitizer:        ${USE_ASAN}")
message(STATUS "UBSanitizer:             ${USE_UBSAN}")
message(STATUS "Profiling:               ${PROFILE_BUILD}")
message(STATUS "Allow unsafe plugins:    ${ALLOW_UNSAFE_PLUGINS}")
message(STATUS "")
