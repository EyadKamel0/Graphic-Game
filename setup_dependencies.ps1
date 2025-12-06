# Setup script for Starwave 3D dependencies
# Run this with: powershell -ExecutionPolicy Bypass -File setup_dependencies.ps1

Write-Host "=== Starwave 3D Dependency Setup ===" -ForegroundColor Cyan
Write-Host ""

$projectRoot = $PSScriptRoot
$externalDir = Join-Path $projectRoot "external"

# Create external directory
if (-not (Test-Path $externalDir)) {
    New-Item -ItemType Directory -Path $externalDir | Out-Null
    Write-Host "Created external directory" -ForegroundColor Green
}

# Function to download file
function Download-File {
    param($url, $output)
    Write-Host "Downloading: $url" -ForegroundColor Yellow
    try {
        Invoke-WebRequest -Uri $url -OutFile $output -UseBasicParsing
        Write-Host "✓ Downloaded successfully" -ForegroundColor Green
        return $true
    } catch {
        Write-Host "✗ Failed to download: $_" -ForegroundColor Red
        return $false
    }
}

Write-Host "This script will help you download the required dependencies." -ForegroundColor White
Write-Host "You'll need to manually extract some files as described below." -ForegroundColor White
Write-Host ""

# GLFW
Write-Host "--- GLFW Setup ---" -ForegroundColor Cyan
$glfwUrl = "https://github.com/glfw/glfw/releases/download/3.3.9/glfw-3.3.9.bin.WIN64.zip"
$glfwZip = Join-Path $externalDir "glfw.zip"
if (-not (Test-Path (Join-Path $externalDir "glfw"))) {
    Write-Host "Download GLFW from: $glfwUrl"
    Write-Host "Then extract to: $externalDir\glfw\" -ForegroundColor Yellow
    Write-Host ""
    $download = Read-Host "Download now? (y/n)"
    if ($download -eq "y") {
        if (Download-File $glfwUrl $glfwZip) {
            Write-Host "Please extract $glfwZip to $externalDir\glfw\" -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "✓ GLFW directory exists" -ForegroundColor Green
}

# stb_image
Write-Host ""
Write-Host "--- stb_image Setup ---" -ForegroundColor Cyan
$stbDir = Join-Path $externalDir "stb"
$stbFile = Join-Path $stbDir "stb_image.h"
if (-not (Test-Path $stbFile)) {
    New-Item -ItemType Directory -Path $stbDir -Force | Out-Null
    $stbUrl = "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
    if (Download-File $stbUrl $stbFile) {
        Write-Host "✓ stb_image.h downloaded" -ForegroundColor Green
    }
} else {
    Write-Host "✓ stb_image.h exists" -ForegroundColor Green
}

# GLM
Write-Host ""
Write-Host "--- GLM Setup ---" -ForegroundColor Cyan
Write-Host "Download GLM from: https://github.com/g-truc/glm/releases"
Write-Host "Extract the glm folder to: $externalDir\glm\" -ForegroundColor Yellow

# GLAD
Write-Host ""
Write-Host "--- GLAD Setup ---" -ForegroundColor Cyan
Write-Host "1. Go to: https://glad.dav1d.de/" -ForegroundColor Yellow
Write-Host "2. Select: Language=C/C++, Specification=OpenGL, API gl=3.3+, Profile=Core" -ForegroundColor Yellow
Write-Host "3. Click Generate and download the ZIP" -ForegroundColor Yellow
Write-Host "4. Extract and copy:" -ForegroundColor Yellow
Write-Host "   - include/glad/ and include/KHR/ to: $externalDir\glad\include\" -ForegroundColor Yellow
Write-Host "   - src/glad.c to: $externalDir\glad\src\" -ForegroundColor Yellow

Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host "After setting up all dependencies, run:" -ForegroundColor White
Write-Host "  cd build" -ForegroundColor Yellow
Write-Host "  cmake .." -ForegroundColor Yellow
Write-Host "  cmake --build ." -ForegroundColor Yellow
Write-Host ""
Write-Host "See SETUP.md for detailed instructions." -ForegroundColor White
