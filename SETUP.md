# Setting Up Dependencies for Starwave 3D

This guide will help you set up the required OpenGL dependencies.

## Required Libraries
- **GLFW** - Window and input management
- **GLAD** - OpenGL function loader
- **GLM** - Math library for graphics
- **stb_image** - Image loading

---

## Option 1: Using Pre-built Libraries (Recommended for Windows)

### Step 1: Download GLFW
1. Go to https://www.glfw.org/download.html
2. Download "Windows pre-compiled binaries" (64-bit)
3. Extract to `external/glfw/` in your project folder

### Step 2: Generate GLAD
1. Go to https://glad.dav1d.de/
2. Select:
   - Language: C/C++
   - Specification: OpenGL
   - API gl: Version 3.3 or higher
   - Profile: Core
3. Click "Generate"
4. Download the ZIP file
5. Extract and copy:
   - `include/glad/` and `include/KHR/` to `external/glad/include/`
   - `src/glad.c` to `external/glad/src/`

### Step 3: Download GLM
1. Go to https://github.com/g-truc/glm/releases
2. Download the latest release
3. Extract the `glm/` folder to `external/glm/`

### Step 4: Download stb_image
1. Go to https://github.com/nothings/stb
2. Download `stb_image.h`
3. Save it to `external/stb/stb_image.h`

### Your folder structure should look like:
```
Graphics Project/
├── external/
│   ├── glfw/
│   │   ├── include/
│   │   │   └── GLFW/
│   │   └── lib-vc2022/  (or lib-vc2019, etc.)
│   │       └── glfw3.lib
│   ├── glad/
│   │   ├── include/
│   │   │   ├── glad/
│   │   │   └── KHR/
│   │   └── src/
│   │       └── glad.c
│   ├── glm/
│   │   └── glm/
│   │       └── (all GLM headers)
│   └── stb/
│       └── stb_image.h
├── shaders/
├── main.cpp
└── ...
```

---

## Option 2: Using vcpkg (Cross-platform)

### Install vcpkg:
```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat  # Windows
./bootstrap-vcpkg.sh   # Linux/Mac
```

### Install packages:
```bash
./vcpkg install glfw3 glad glm
./vcpkg integrate install
```

### Build with vcpkg:
```bash
cd "C:/Users/WinDows/Desktop/Graphics Project/build"
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake -DUSE_VCPKG=ON
cmake --build .
```

---

## Building the Project

### After setting up dependencies:

```bash
cd "C:/Users/WinDows/Desktop/Graphics Project/build"
cmake ..
cmake --build .
```

### Or with Visual Studio:
```bash
cmake .. -G "Visual Studio 17 2022"  # Adjust version as needed
```
Then open the generated .sln file in Visual Studio.

---

## Quick Fix: If you just want to test compilation

If you already have the libraries installed somewhere on your system, you can:

1. Edit `CMakeLists.txt` and update the paths in the "Manual dependency paths" section
2. Point to where you have GLFW, GLAD, GLM, and stb_image installed
3. Update the library path (e.g., `glfw3.lib` location)

---

## Troubleshooting

**"Cannot find GLFW"**
- Verify `external/glfw/include/GLFW/glfw3.h` exists
- Check that lib file matches your Visual Studio version (lib-vc2022, lib-vc2019, etc.)

**"Cannot find glad.h"**
- Make sure you generated GLAD for OpenGL 3.3+ Core profile
- Verify files are in `external/glad/include/glad/glad.h`

**"Undefined reference to glad functions"**
- Ensure `glad.c` is in `external/glad/src/` and CMake will compile it

**Link errors**
- Update the GLFW library path in CMakeLists.txt to match your VS version
- Common paths: `lib-vc2022`, `lib-vc2019`, `lib-vc2017`, `lib-mingw-w64`
