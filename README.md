# Minecraft Beta 1.7.3 Server - Modern C++ Implementation

A high-performance, cross-platform reimplementation of the Minecraft Beta 1.7.3 server in modern C++23/C++20.

## Features

- **Cross-platform**: Windows, Linux, macOS
- **High performance**: Multithreaded architecture, data-oriented design
- **Low memory**: Arena allocators, object pools, minimal allocations
- **Modern C++**: C++23 preferred, C++20 minimum, RAII throughout
- **Beta 1.7.3 Protocol**: Full network protocol compatibility
- **Extensible**: Plugin SDK (Step 3)

## Quick Start

### Building on Windows

```batch
# Using Visual Studio 2022
scripts\build_windows.bat Release

# Or using PowerShell
powershell -ExecutionPolicy Bypass -File scripts\build_windows.ps1 -Configuration Release
```

The build will generate a Visual Studio solution at `build/windows/MinecraftBeta173Server.sln`.

### Building on Linux/macOS

```bash
# Debug build
./scripts/build_unix.sh debug

# Release build
./scripts/build_unix.sh release
```

### Running the Server

```bash
# Windows
out\Windows-x64\Release\mcserver.exe

# Linux/macOS
out/Linux-x86_64/Release/mcserver
```

The server will create a `server.properties` file on first run. Edit this file to configure the server.

## Configuration

Edit `server.properties` to configure server settings:

```properties
server-ip=
server-port=25565
level-name=world
level-seed=
online-mode=false
spawn-animals=true
spawn-monsters=true
pvp=true
allow-flight=false
allow-nether=true
max-players=20
```

## Development

### IDE Support

- **Visual Studio**: Open `build/windows/MinecraftBeta173Server.sln`
- **CLion**: Open project root (uses CMakePresets.json)
- **VSCode**: Install CMake Tools extension

### Running Tests

```bash
# After building
ctest --output-on-failure

# Or run directly
out/Linux-x86_64/Debug/tests_unit
```

### Build Options

```bash
cmake -DBUILD_TESTS=ON          # Enable tests (default: ON)
cmake -DUSE_ASAN=ON             # Enable AddressSanitizer
cmake -DUSE_UBSAN=ON            # Enable UndefinedBehaviorSanitizer
cmake -DPROFILE_BUILD=ON        # Enable profiling
cmake -DALLOW_UNSAFE_PLUGINS=ON # Disable plugin sandbox (Step 3)
```

## Performance

- **Target**: 20 TPS (50ms per tick)
- **Architecture**: Data-oriented design with structure-of-arrays for hot paths
- **Memory**: Arena allocators for per-tick temporaries, object pools for packets/entities
- **Parallelism**: Job system for chunk generation, packet encoding, I/O staging

## License

This is a clean-room reimplementation based on decompiled sources for educational purposes.

## Acknowledgments

- Minecraft Beta 1.7.3 decompiled sources