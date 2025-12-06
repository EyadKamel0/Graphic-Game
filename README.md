# Starwave 3D

A 3D re-imagining of the classic Asteroids game built with C++ and OpenGL.

## Project Structure

```
Graphics Project/
├── main.cpp              # Entry point
├── Game.h/cpp           # Main game class (window, loop, rendering)
├── PlayerShip.h/cpp     # Player ship with controls
├── Camera.h             # Camera utility class
├── Shader.h             # Shader loading utility
├── Texture.h            # Texture loading utility
├── shaders/
│   ├── textured.vert    # Vertex shader
│   └── textured.frag    # Fragment shader
└── textures/            # TODO: Add your textures here
    ├── ship.png         # Ship texture
    └── space_grid.png   # Space plane texture
```

## Controls

- **W** - Move forward (accelerate)
- **S** - Slow down (decelerate)
- **A** - Rotate left (yaw counter-clockwise)
- **D** - Rotate right (yaw clockwise)
- **ESC** - Quit game

## Building the Project

### Prerequisites

You'll need the following libraries:
- **GLFW** - Window and input handling
- **GLAD** - OpenGL function loader
- **GLM** - Mathematics library
- **stb_image** - Image loading

### Build Instructions

1. Update the `CMakeLists.txt` file with the correct paths to your libraries
2. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```
3. Run CMake:
   ```bash
   cmake ..
   ```
4. Build the project:
   ```bash
   cmake --build .
   ```

### Texture Setup

Create a `textures/` folder and add:
- `ship.png` - Texture for the player ship
- `space_grid.png` - Texture for the space plane

The code will use magenta placeholder textures if files are not found.

## Technical Details

- **Graphics API**: Modern OpenGL 3.3+ (Core Profile)
- **Rendering**: VAO/VBO-based mesh rendering
- **Shading**: Custom textured shader with Phong lighting
- **Camera**: Third-person follow camera behind the ship
- **Movement**: Physics-based acceleration/deceleration with rotation

## Current Features

✅ Basic game loop with delta time  
✅ Player ship with textured cube representation  
✅ Keyboard controls (WASD)  
✅ Large space plane for testing movement  
✅ Third-person camera that follows the ship  
✅ Basic Phong lighting  
✅ Texture support with fallback placeholders  

## Next Steps

- Add asteroid spawning and collision
- Implement shooting mechanics
- Create two game levels
- Add particle effects
- Implement scoring system
- Add sound effects and music

## Notes

- Forward direction is -Z in world space
- Ship rotates around Y-axis (yaw)
- Camera automatically follows ship from behind
- All code uses modern OpenGL (no deprecated immediate mode)
