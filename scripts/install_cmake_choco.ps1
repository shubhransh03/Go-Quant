# Install CMake using Chocolatey (requires admin privileges)
# Usage: Open an elevated PowerShell (Run as Administrator) and execute:
#    .\scripts\install_cmake_choco.ps1

if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Host "Chocolatey not found. Installing Chocolatey is outside the scope of this script. Please install Chocolatey first: https://chocolatey.org/install"
    exit 1
}

# Install cmake
choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y

Write-Host "CMake installation requested. After install completes, restart your shell and run the build steps:"
Write-Host "  if (-Not (Test-Path build)) { New-Item -ItemType Directory -Path build }"
Write-Host "  Set-Location build"
Write-Host "  cmake .. -DCMAKE_BUILD_TYPE=Release"
Write-Host "  cmake --build . --config Release -- /m"
