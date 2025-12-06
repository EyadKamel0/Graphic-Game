@echo off
echo Starwave 3D - Visual Studio Build Script
echo ==========================================
echo.

cd /d "%~dp0"

REM Find Visual Studio installation
set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: Could not find Visual Studio
    echo Please install Visual Studio 2019 or later with C++ support
    pause
    exit /b 1
)

REM Get Visual Studio installation path
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VSINSTALL=%%i"
)

if not defined VSINSTALL (
    echo ERROR: Could not find Visual Studio with C++ tools
    echo Please install C++ development tools in Visual Studio
    pause
    exit /b 1
)

REM Set up Visual Studio environment
echo Setting up Visual Studio environment...
call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to set up Visual Studio environment
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

REM Run CMake with Visual Studio generator
echo.
echo Running CMake configuration...
cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

REM Build the project
echo.
echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ===================================
echo Build successful!
echo ===================================
echo.
echo The executable is at: build\Release\Starwave3D.exe
echo.
echo Running the game...
echo.

REM Run the game
if exist "Release\Starwave3D.exe" (
    cd Release
    Starwave3D.exe
) else (
    echo ERROR: Could not find executable
    pause
)
