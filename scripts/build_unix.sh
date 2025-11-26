#!/bin/bash
# Build script for Linux/macOS
# Usage: ./scripts/build_unix.sh [debug|release] [clean]

set -e

CONFIG="${1:-debug}"
CLEAN="${2}"

if [[ "$CONFIG" == "release" ]]; then
    CMAKE_BUILD_TYPE="Release"
    CONFIG_LOWER="release"
else
    CMAKE_BUILD_TYPE="Debug"
    CONFIG_LOWER="debug"
fi

echo "=== Minecraft Beta 1.7.3 Server - Unix Build ==="
echo "Configuration: $CMAKE_BUILD_TYPE"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

# Detect platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
    OS_NAME="macOS"
else
    PLATFORM="linux"
    OS_NAME="Linux"
fi

BUILD_DIR="$ROOT_DIR/build/$PLATFORM"
OUT_DIR="$ROOT_DIR/out/$OS_NAME-$(uname -m)/$CMAKE_BUILD_TYPE"

# Clean if requested
if [[ "$CLEAN" == "clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    rm -rf "$OUT_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

# Detect generator
if command -v ninja &> /dev/null; then
    GENERATOR="Ninja"
    echo "Using Ninja build system"
else
    GENERATOR="Unix Makefiles"
    echo "Using Make build system"
fi

# Configure CMake
echo "Configuring CMake..."
cd "$BUILD_DIR"
cmake -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    -DBUILD_TESTS=ON \
    "$ROOT_DIR"

# Build
echo "Building..."
cmake --build . --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

echo ""
echo "=== Build completed successfully ==="
echo "Executable: $OUT_DIR/mcserver"

# Run tests if they exist
if [[ -f "$BUILD_DIR/tests/tests_unit" ]]; then
    echo ""
    echo "Running tests..."
    ctest --output-on-failure
fi
