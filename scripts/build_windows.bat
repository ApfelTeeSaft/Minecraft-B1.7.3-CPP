@echo off
REM Build script for Windows
REM Usage: scripts\build_windows.bat [Debug|Release]
REM Note: Must be run from Windows CMD, not Git Bash/WSL

setlocal enabledelayedexpansion

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

echo === Minecraft Beta 1.7.3 Server - Windows Build ===
echo Configuration: %CONFIG%

REM Get script directory and project root
set "SCRIPT_DIR=%~dp0"
REM Remove trailing backslash and go up one directory
for %%i in ("%SCRIPT_DIR%..") do set "ROOT_DIR=%%~fi"

echo Root directory: %ROOT_DIR%

REM Check if CMakeLists.txt exists
if not exist "%ROOT_DIR%\CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found in %ROOT_DIR%
    echo This script must be run from the scripts directory
    exit /b 1
)

set "BUILD_DIR=%ROOT_DIR%\build\windows"
set "OUT_DIR=%ROOT_DIR%\out\Windows-x64\%CONFIG%"

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Detect generator
where ninja >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using Ninja build system
    set "GENERATOR=Ninja"
) else (
    where cl >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo Using Visual Studio 17 2022
        set "GENERATOR=Visual Studio 17 2022"
    ) else (
        echo ERROR: No suitable build system found
        echo Please install Visual Studio 2022 or Ninja
        exit /b 1
    )
)

REM Configure CMake
echo.
echo Configuring CMake...
cd /d "%BUILD_DIR%"

if "%GENERATOR%"=="Ninja" (
    cmake -G "Ninja" -DCMAKE_BUILD_TYPE=%CONFIG% -DBUILD_TESTS=ON "%ROOT_DIR%"
) else (
    cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=%CONFIG% -DBUILD_TESTS=ON "%ROOT_DIR%"
)

if errorlevel 1 (
    echo.
    echo CMake configuration failed!
    exit /b 1
)

REM Build
echo.
echo Building...
cmake --build . --config %CONFIG% --parallel

if errorlevel 1 (
    echo.
    echo Build failed!
    exit /b 1
)

echo.
echo === Build completed successfully ===
echo Executable: %OUT_DIR%\mcserver.exe
if "%GENERATOR%"=="Visual Studio 17 2022" (
    echo Visual Studio Solution: %BUILD_DIR%\MinecraftBeta173Server.sln
)

endlocal
