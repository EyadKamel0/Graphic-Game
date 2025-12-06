@echo off
echo Building Starwave 3D...
echo.

cd /d "%~dp0"

REM Check if CMake is available
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake is not installed or not in PATH
    echo Please install CMake from https://cmake.org/download/
    echo Or use Visual Studio which includes CMake
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

REM Run CMake
echo Running CMake configuration...
cmake .. -G "MinGW Makefiles"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo CMake configuration failed. Trying with Visual Studio...
    cmake .. -G "Visual Studio 17 2022"
    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo ERROR: CMake configuration failed
        echo Make sure you have a C++ compiler installed
        pause
        exit /b 1
    )
)

REM Build the project
echo.
echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ===================================
echo Build successful!
echo ===================================
echo.
echo To run the game:
echo   cd build
if exist "Release\Starwave3D.exe" (
    echo   Release\Starwave3D.exe
) else (
    echo   Starwave3D.exe
)
echo.
pause
