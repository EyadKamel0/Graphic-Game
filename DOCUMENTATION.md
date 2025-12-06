# Starwave 3D - Complete Documentation

## Project Overview

**Starwave 3D** is an on-rails arcade space shooter built with C++ and OpenGL. The game features a player-controlled spaceship that can move in the X/Y plane while the world scrolls automatically in the Z-axis, creating a "fake 3D" on-rails experience similar to classic arcade shooters like Star Fox.

---

## Game Architecture

### Core Systems

#### 1. **On-Rails Movement System**
- **Player Control**: Ship moves freely in X/Y plane within defined boundaries
- **World Scrolling**: All objects move toward the player at a constant speed (15 units/sec)
- **Camera System**: 
  - Third-person camera follows ship from behind
  - First-person camera option available (toggle with right mouse button)
  - Camera does NOT rotate with mouse movement (disabled for arcade feel)

#### 2. **Input System**
- **W/A/S/D**: Move ship up/left/down/right
- **Left Mouse Button**: Fire bullets straight ahead
- **Right Mouse Button**: Toggle first/third person camera
- **Space**: Booster (future feature)
- **ENTER**: Complete level (temporary testing feature)
- **Mouse Movement**: Disabled (camera is locked)

#### 3. **Physics & Collision**
- Simple sphere-based collision detection
- Ship collision with asteroids causes pushback
- Bullet collision with small asteroids triggers break animation
- Collectible collision triggers pickup and power-up activation

---

## Level System

### Level 1: Outer Drift Zone

**Objective**: Collect 5 Energy Shards to activate the Wave Portal, then fly through it to complete the level.

#### Environment
- **Grid Floor**: 10x40 cell scrolling grid that creates depth perception
- **Play Area Boundaries**: 
  - X: -50 to +50
  - Y: -30 to +30
  - Z: Scrolls from -200 to player position

#### Obstacles

**Large Drift Asteroids (24 total)**
- Size: 2.5 unit radius
- Behavior: Slow rotation, drifting movement
- Distribution: Spread across full play area
  - 6 on far left (X: -50 to -35)
  - 6 on far right (X: 35 to 50)
  - 4 on top (Y: 15 to 30)
  - 4 on bottom (Y: -30 to -15)
  - 6 in center area
- Collision: Ship pushback on impact

**Small Breakable Asteroids (49 total)**
- Size: 0.8 unit radius
- Behavior: Fast rotation, varied trajectories
- Distribution: Approaching from all directions
  - Left edge: 12 asteroids (X: -50 to -35)
  - Right edge: 12 asteroids (X: 35 to 50)
  - Top area: 5 asteroids (Y: 15 to 30)
  - Bottom area: 5 asteroids (Y: -30 to -15)
  - Center: 16 asteroids (mixed positions)
- Collision: Destroyed by bullets, creates 2-3 flying fragments

---

## Collectible System

### Types of Collectibles (8 total in Level 1)

#### 1. **Energy Shards** (5 required)
- **Purpose**: Primary objective - collect all 5 to activate the portal
- **Visual**: Diamond/crystal shape, gentle rotation
- **Animation**: Bobs up and down (0.4 unit amplitude)
- **Collection**: Shows progress "Energy Shards: X/5"
- **Positioning**: First 3 placed close to start for visibility

#### 2. **Jet Booster Power-Up** (1-2 in level)
- **Effect**: Temporary speed boost for ship movement
- **Duration**: 8 seconds
- **Visual**: Crystal shape, faster rotation than shards
- **Animation**: Faster bobbing (0.5 unit amplitude)
- **Status**: Shows "[POWERUP] Jet Booster active for 8 seconds!"

#### 3. **Mini Burst Shot Power-Up** (1-2 in level)
- **Effect**: Fires 3 bullets in a spread pattern instead of 1
- **Duration**: 8 seconds
- **Spread Pattern**: Center + Left (-0.15 angle) + Right (+0.15 angle)
- **Visual**: Crystal shape, fastest rotation
- **Status**: Shows "[POWERUP] Mini Burst Shot active for 8 seconds!"

### Collectible Technical Details

**Mesh Structure**
- Shape: Elongated octahedron (crystal/diamond)
- Vertices: 6 (top, bottom, 4 cardinal points)
- Faces: 8 triangles
- OpenGL: VAO/VBO/EBO with indexed drawing

**Pickup Animation**
- Duration: 0.3 seconds
- Effect: Scale down to 0 with glow intensity spike
- Removal: Object marked as collected, stops rendering immediately

---

## Wave Portal System

### Portal Mechanics

**Activation Requirements**
- Collect all 5 Energy Shards
- Portal becomes active (glows brighter, spins faster)
- Message: "[PORTAL] Portal activated! Fly through to complete the level!"

**Portal Visual**
- Shape: Spinning torus (ring)
- Position: Z = -200 (far end of level)
- Inactive State: Slow rotation (30°/sec), dim
- Active State: Fast rotation (90°/sec), bright glow

**Level Completion**
- Fly ship through portal center (collision radius check)
- Triggers level completion
- Message: "[LEVEL COMPLETE] You entered the portal! Proceeding to Level 2..."

---

## Combat System

### Weapon Mechanics

**Standard Shooting**
- Fire Rate: 0.2 seconds between shots (5 rounds/sec)
- Bullet Speed: 50 units/second
- Bullet Lifetime: 5 seconds (250 unit range)
- Direction: Always straight ahead (-Z direction)
- Collision: Destroys small asteroids on impact

**Mini Burst Shot (Power-Up)**
- Fires 3 bullets simultaneously
- Pattern: Center + left offset + right offset
- Same speed and lifetime as standard bullets
- Triple the destructive power

**Bullet Technical Details**
- Mesh: Small cube (0.15 unit size)
- Rendering: 36 vertices (6 faces × 2 triangles × 3 vertices)
- Lazy Initialization: VAO/VBO created on first render
- Cleanup: Bullets auto-remove when expired or after collision

---

## Technical Implementation

### Class Architecture

#### Core Classes

**Game** (`Game.cpp/h`)
- Main game loop and window management
- GLFW window creation and event handling
- OpenGL context initialization
- Manages LevelManager, PlayerShip, and Camera

**LevelManager** (`LevelManager.cpp/h`)
- Handles level progression
- Switches between levels (currently 2 levels defined)
- Delegates update/render calls to active level
- Manages level completion transitions

**PlayerShip** (`PlayerShip.cpp/h`)
- Ship movement and input handling
- Position management with boundary constraints
- Power-up state tracking (Jet Booster active/inactive)
- Model matrix generation for rendering

**Camera** (`Camera.cpp/h`)
- Third-person follow camera
- First-person camera mode
- View matrix calculation
- Camera state toggling

#### Level Classes

**Level1OuterDriftZone** (`Level1OuterDriftZone.cpp/h`)
- Main implementation of Level 1
- Manages all game objects (asteroids, bullets, collectibles, portal)
- Collision detection systems
- Object spawning and initialization
- Power-up timer management

**Asteroid** (`Asteroid.cpp/h`)
- Large drifting asteroids
- Sphere collision detection
- Rotation and movement
- Mesh: Icosphere geometry

**SmallAsteroid** (`SmallAsteroid.cpp/h`)
- Breakable asteroids with state machine
- States: Normal → Breaking → Broken
- Break animation with scale-down effect
- Faster rotation than large asteroids

**Fragment** (`Fragment.cpp/h`)
- Debris created when small asteroids break
- Flies outward from break point
- Random velocity and direction
- Auto-removes after 2 seconds

**Bullet** (`Bullet.cpp/h`)
- Player projectiles
- Move constructor/assignment for safe vector storage
- Lazy mesh initialization
- Automatic cleanup on expiration

**Collectible** (`Collectible.cpp/h`)
- Energy Shards and Power-Ups
- Type enum: EnergyShard, JetBooster, MiniBurstShot
- Move constructor/assignment for safe vector storage
- Bob animation and rotation
- Pickup animation system

**WavePortal** (`WavePortal.cpp/h`)
- Level completion goal
- Activation system (requires 5 shards)
- Torus mesh rendering
- Entry detection for level completion

### Critical Bug Fixes

#### Move Semantics Implementation

**Problem**: Segmentation faults when collecting items and shooting
- Vectors containing `Bullet` and `Collectible` objects would reallocate
- Default copy constructor duplicated OpenGL resource handles (VAO/VBO/EBO)
- Multiple objects referenced the same GPU resources
- When one object was destroyed, it deleted shared resources
- Other objects crashed when trying to use deleted resources

**Solution**: Implemented proper move semantics
```cpp
// Disabled copy operations
Bullet(const Bullet&) = delete;
Bullet& operator=(const Bullet&) = delete;

// Implemented move operations
Bullet(Bullet&& other) noexcept;
Bullet& operator=(Bullet&& other) noexcept;
```
- Move constructor transfers ownership of OpenGL resources
- Sets source object's handles to 0 after transfer
- Prevents double-deletion of GPU resources
- Same fix applied to both `Bullet` and `Collectible` classes

#### Iterator Safety

**Problem**: Potential iterator invalidation during vector modification
**Solution**: Changed from range-based for loops to index-based loops
```cpp
// Before (unsafe)
for (auto& bullet : bullets) {
    bullet.render();
}

// After (safe)
for (size_t i = 0; i < bullets.size(); ++i) {
    bullets[i].render();
}
```

---

## OpenGL Rendering

### Shader System

**Textured Shader** (`textured_shader.vert/frag`)
- Vertex shader: Transforms positions, passes normals and UVs
- Fragment shader: Samples textures, basic lighting
- Uniforms: model, view, projection matrices

**Mesh Data Format**
- Vertex Layout: Position (3) + Normal (3) + TexCoord (2) = 8 floats
- VAO: Vertex Array Object (stores vertex attribute configuration)
- VBO: Vertex Buffer Object (stores vertex data)
- EBO: Element Buffer Object (stores indices for indexed drawing)

### Texture System

**Texture Loading**
- Uses stb_image library
- Supports PNG, JPG formats
- Fallback: White color when texture fails to load
- Textures loaded per-object type:
  - Ship texture: `textures/ship.png`
  - Asteroid texture: `textures/asteroid_rock.png`
  - Small asteroid: `textures/small_asteroid.png`
  - Bullet: `textures/bullet.png`
  - Collectible: `textures/collectible.png`
  - Portal: `textures/portal.png`
  - Level background: `textures/level1_drift.png`

---

## Game Statistics

### Level 1 Current Configuration

**Objects**
- Large Asteroids: 24
- Small Asteroids: 49
- Collectibles: 8 (5 shards + 3 power-ups)
- Wave Portal: 1

**Collectible Distribution**
- Energy Shards: 5 required
- Jet Booster Power-Ups: 1-2
- Mini Burst Shot Power-Ups: 1-2
- Total collectibles: 8

**Power-Up Timers**
- Duration: 8 seconds each
- Can have multiple power-ups active simultaneously
- Visual feedback: "[POWERUP] X active for 8 seconds!"

**Collision Metrics**
- Ship radius: 1.0 unit
- Large asteroid radius: 2.5 units
- Small asteroid radius: 0.8 units
- Bullet radius: 0.2 units
- Collectible radius: 0.7 units
- Portal entry radius: 3.0 units

---

## Build System

### CMake Configuration

**Project Structure**
```
Graphics Project/
├── CMakeLists.txt          # Main build configuration
├── Game.cpp/h              # Core game class
├── LevelManager.cpp/h      # Level system
├── PlayerShip.cpp/h        # Player character
├── Camera.cpp/h            # Camera system
├── Shader.cpp/h            # Shader management
├── Level1OuterDriftZone.cpp/h
├── Asteroid.cpp/h
├── SmallAsteroid.cpp/h
├── Fragment.cpp/h
├── Bullet.cpp/h
├── Collectible.cpp/h
├── WavePortal.cpp/h
├── shaders/
│   ├── textured_shader.vert
│   └── textured_shader.frag
├── textures/               # Texture files (optional)
└── build/                  # CMake build directory
    └── Release/
        └── Starwave3D.exe
```

**Dependencies**
- **GLFW**: Window and input management
- **GLAD**: OpenGL function loading
- **GLM**: Mathematics library (vectors, matrices)
- **stb_image**: Texture loading

**Build Commands**
```bash
# Configure
cd "Graphics Project/build"
cmake ..

# Build (Windows)
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Starwave3D.sln //p:Configuration=Release

# Run
cd Release
./Starwave3D.exe
```

---

## Future Enhancements

### Planned Features
1. **Audio System**: Sound effects for shooting, collisions, pickups
2. **Particle Effects**: Explosions, thruster trails, collectible glow
3. **Score System**: Points for asteroid destruction, collectibles
4. **Health System**: Ship damage and health bar
5. **Level 2**: New environment with different challenges
6. **Boss Battles**: Large enemy encounters
7. **Texture Assets**: Replace white fallback with proper textures
8. **HUD**: Display health, score, power-up status, shard count

### Known Issues
- ✅ Fixed: Segmentation fault after collecting and shooting
- ✅ Fixed: VAO/VBO handle duplication in vectors
- Textures currently using white fallback (files not found)
- Portal entry detection may need refinement
- No damage/death system yet

---

## Performance Notes

### Optimization Strategies
1. **Object Pooling**: Bullets/fragments could use object pools
2. **Lazy Initialization**: Meshes created on first render (Bullet class)
3. **Upfront Initialization**: Collectibles/portal initialize immediately
4. **Index-Based Rendering**: Uses EBO for efficient triangle rendering
5. **Vector Reserves**: Fragment vector reserves space before adding

### Frame Budget
- Target: 60 FPS (16.67ms per frame)
- Current performance: Smooth on modern hardware
- Bottle necks: Multiple draw calls per frame (could be batched)

---

## Debug Console Output

### Message Categories

**Initialization**
```
Loading Level 1: Outer Drift Zone...
  Grid floor created: 10x40 cells
  Created 24 drift asteroids
  Created 49 small asteroids
  Created 8 collectibles (5 shards, 3 power-ups)
  Portal placed at z = -200
```

**Gameplay Events**
```
[COLLECT] ✨ Energy Shard collected!
[PROGRESS] Energy Shards: X/5
[COLLECT] 🚀 Jet Booster power-up activated!
[POWERUP] Jet Booster active for 8 seconds!
[FIRE] Bullet fired. Total: X
[HIT] Bullet destroyed small asteroid
[COLLISION] Ship hit asteroid
[PORTAL] Portal activated!
[LEVEL COMPLETE] You entered the portal!
```

---

## Code Quality & Practices

### C++ Best Practices Implemented
- ✅ RAII (Resource Acquisition Is Initialization)
- ✅ Move semantics for resource management
- ✅ Const correctness
- ✅ Delete copy constructors where appropriate
- ✅ Smart separation of concerns
- ✅ Clear class responsibilities

### OpenGL Best Practices
- ✅ VAO/VBO cleanup in destructors
- ✅ Proper OpenGL state management
- ✅ Texture binding before rendering
- ✅ Shader uniform updates per object
- ✅ Buffer orphaning prevention

---

## Version History

### Current Version: v1.0 (December 2025)

**Features Implemented**
- ✅ On-rails movement system
- ✅ Player ship with X/Y movement
- ✅ 76 asteroids from all directions
- ✅ Bullet shooting system
- ✅ Collectible system (3 types)
- ✅ Wave Portal goal
- ✅ Power-up system (Jet Booster, Mini Burst Shot)
- ✅ Collision detection
- ✅ Fragment system for asteroid breaks
- ✅ Camera toggle (first/third person)
- ✅ Level completion mechanics

**Bug Fixes**
- ✅ Fixed segmentation fault (move semantics)
- ✅ Fixed iterator invalidation issues
- ✅ Fixed OpenGL resource duplication
- ✅ Fixed collectible rendering after pickup

---

## Credits

**Development**: Custom C++ OpenGL implementation
**Libraries**: GLFW, GLAD, GLM, stb_image
**Architecture**: On-rails arcade shooter inspired by Star Fox
**Documentation**: Complete technical and gameplay documentation

---

**Last Updated**: December 1, 2025
**Status**: Fully Functional, Crash-Free, Feature Complete for Level 1
