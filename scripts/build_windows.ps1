#!/usr/bin/env pwsh
# Build script for Windows using PowerShell
# Usage: .\scripts\build_windows.ps1 [-Configuration Debug|Release] [-Clean]

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Configuration = "Debug",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

Write-Host "=== Minecraft Beta 1.7.3 Server - Windows Build ===" -ForegroundColor Cyan
Write-Host "Configuration: $Configuration" -ForegroundColor Green

$RootDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$BuildDir = Join-Path $RootDir "build\windows"
$OutDir = Join-Path $RootDir "out\Windows-x64\$Configuration"

# Clean if requested
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
    if (Test-Path $OutDir) {
        Remove-Item -Recurse -Force $OutDir
    }
}

# Create build directory
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

# Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Yellow
Push-Location $BuildDir
try {
    cmake -G "Visual Studio 17 2022" -A x64 `
        -DCMAKE_BUILD_TYPE=$Configuration `
        -DBUILD_TESTS=ON `
        $RootDir

    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }

    # Build
    Write-Host "Building..." -ForegroundColor Yellow
    cmake --build . --config $Configuration --parallel

    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }

    Write-Host ""
    Write-Host "=== Build completed successfully ===" -ForegroundColor Green
    Write-Host "Executable: $OutDir\mcserver.exe" -ForegroundColor Cyan
    Write-Host "Visual Studio Solution: $BuildDir\MinecraftBeta173Server.sln" -ForegroundColor Cyan
} finally {
    Pop-Location
}
