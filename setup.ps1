# Install required dependencies for the matching engine

# Check if running with administrator privileges
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Warning "Please run this script as Administrator!"
    Exit 1
}

# Function to check if a command exists
function Test-CommandExists {
    param ($command)
    $oldPreference = $ErrorActionPreference
    $ErrorActionPreference = 'stop'
    try {
        if (Get-Command $command) { return $true }
    } catch {
        return $false
    } finally {
        $ErrorActionPreference = $oldPreference
    }
}

# Install Chocolatey if not installed
if (-not (Test-CommandExists "choco")) {
    Write-Host "Installing Chocolatey..."
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
    refreshenv
}

# Install Visual Studio Build Tools if not installed
if (-not (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools")) {
    Write-Host "Installing Visual Studio Build Tools..."
    choco install visualstudio2019buildtools -y
    choco install visualstudio2019-workload-vctools -y
}

# Install CMake if not installed
if (-not (Test-CommandExists "cmake")) {
    Write-Host "Installing CMake..."
    choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y
    refreshenv
}

# Install Git if not installed
if (-not (Test-CommandExists "git")) {
    Write-Host "Installing Git..."
    choco install git -y
    refreshenv
}

# Install vcpkg if not present
$vcpkgPath = "C:\vcpkg"
if (-not (Test-Path $vcpkgPath)) {
    Write-Host "Installing vcpkg..."
    git clone https://github.com/Microsoft/vcpkg.git $vcpkgPath
    Push-Location $vcpkgPath
    .\bootstrap-vcpkg.bat
    .\vcpkg integrate install
    Pop-Location
}

# Add vcpkg to PATH if not already there
$env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
if (-not ($env:Path -like "*vcpkg*")) {
    [Environment]::SetEnvironmentVariable(
        "Path",
        [Environment]::GetEnvironmentVariable("Path", "Machine") + ";C:\vcpkg",
        "Machine"
    )
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine")
}

# Install required packages using vcpkg
Write-Host "Installing project dependencies..."
vcpkg install boost:x64-windows
vcpkg install nlohmann-json:x64-windows
vcpkg install gtest:x64-windows
vcpkg install websocketpp:x64-windows
vcpkg install asio:x64-windows

Write-Host "Setting up build directory..."
$buildDir = "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir
}

Push-Location $buildDir

Write-Host "Configuring project with CMake..."
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -A x64

Write-Host "Building project..."
cmake --build . --config Release

Pop-Location

Write-Host "Setup complete! You can now build the project using Visual Studio or run the executables in the build/Release directory."