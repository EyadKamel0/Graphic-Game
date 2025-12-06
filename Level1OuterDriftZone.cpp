#include "Level1OuterDriftZone.h"
#include "Game.h"
#include "Texture.h"
#include "Shader.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

/*
 * ═══════════════════════════════════════════════════════════════
 *  LEVEL 1: OUTER DRIFT ZONE - SCROLL CONFIGURATION
 * ═══════════════════════════════════════════════════════════════
 * 
 * On-rails scrolling: World moves toward the ship along +Z axis to create
 * the illusion of forward movement. Ship stays at fixed Z position.
 * 
 * All dynamic objects (asteroids, collectibles, portal) scroll at this rate.
 * Bullets move independently in world space and are NOT scrolled.
 */

// Level 1 world scroll speed (units per second along +Z toward player)
const float L1_SCROLL_SPEED = 15.0f;

// Infinite spawn system constants - Play area boundaries
const float Level1OuterDriftZone::L1_PLAY_AREA_MIN_X = -50.0f;
const float Level1OuterDriftZone::L1_PLAY_AREA_MAX_X = 50.0f;
const float Level1OuterDriftZone::L1_PLAY_AREA_MIN_Y = -30.0f;
const float Level1OuterDriftZone::L1_PLAY_AREA_MAX_Y = 30.0f;
const float Level1OuterDriftZone::L1_SPAWN_DISTANCE_AHEAD = 150.0f;  // Spawn 150 units ahead of camera

Level1OuterDriftZone::Level1OuterDriftZone()
    : skybox(nullptr), planeVAO(0), planeVBO(0), planeTexture(0), asteroidTexture(0), 
      smallAsteroidTexture(0), bulletTexture(0), collectibleTexture(0), portalTexture(0),
      planeVertexCount(0), completed(false), gameOver(false), levelTimer(0.0f),
      lastFireTime(0.0f), fireRate(0.15f), score(0),
      scrollSpeed(L1_SCROLL_SPEED), worldZOffset(0.0f),
      collectedShardCount(0), requiredShardCount(L1_REQUIRED_SHARDS),
      spawnedShards(0), spawnedPowerups(0),
      distanceTraveled(0.0f), nextCollectibleSpawnDistance(40.0f),
      portal(nullptr),
      portalBeingCaptured(false),  // Not captured yet
      jetBoosterActive(false), jetBoosterEndTime(0.0f),
      miniBurstActive(false), miniBurstEndTime(0.0f),
      powerupDuration(8.0f),  // Power-ups last 8 seconds
      hudVAO(0), hudVBO(0), hudShaderProgram(0) {
}

Level1OuterDriftZone::~Level1OuterDriftZone() {
    cleanup();
}

void Level1OuterDriftZone::init(Game& game) {
    std::cout << "Loading Level 1: Outer Drift Zone..." << std::endl;
    
    // Initialize skybox
    skybox = new Skybox();
    std::vector<std::string> faces = {
        "blue/bkg1_right.png",   // +X
        "blue/bkg1_left.png",    // -X
        "blue/bkg1_top.png",     // +Y
        "blue/bkg1_bot.png",     // -Y
        "blue/bkg1_front.png",   // +Z
        "blue/bkg1_back.png"     // -Z
    };
    skybox->loadCubemap(faces);
    
    setupPlane();
    Collectible::loadSharedMeshes();     // Load shared OBJ model for collectibles
    initAsteroidSpawner();       // NEW: Infinite asteroid spawn system
    initSmallAsteroidSpawner();  // NEW: Infinite small asteroid spawn system
    initCollectibleSpawner();    // NEW: Distance-based collectible spawning
    completed = false;
    gameOver = false;
    levelTimer = 0.0f;
    lastFireTime = 0.0f;
    collectedShardCount = 0;
    portalBeingCaptured = false;
    distanceTraveled = 0.0f;
    nextCollectibleSpawnDistance = 40.0f;  // First collectible after 40 units
    
    // Reset player ship health
    game.getPlayerShip()->resetHealth();
    
    // Clear any leftover projectiles
    bullets.clear();
    fragments.clear();
    
    // Reserve generous capacity to completely avoid reallocation during gameplay
    // With ~50 small asteroids, each creating 2-3 fragments, worst case is ~150 fragments
    // Reserve 500 to be absolutely safe and prevent any reallocations
    fragments.reserve(500);
    bullets.reserve(100);  // Also reserve for bullets to prevent reallocation
    
    // Initialize HUD state
    hudState = Level1HUDState();
    hudState.totalShards = requiredShardCount;
    
    // Store screen dimensions for HUD centering
    hudScreenWidth = game.getScreenWidth();
    hudScreenHeight = game.getScreenHeight();
    
    // Initialize 2D HUD rendering
    setupHUD2D(hudScreenWidth, hudScreenHeight);
    
    // Initialize win screen text renderer (also used for power-up labels)
    winTextRenderer.init(hudScreenWidth, hudScreenHeight);
    
    std::cout << "  [INFINITE SPAWN] Asteroids will spawn continuously (70% large, 30% small)" << std::endl;
    std::cout << "  [COLLECTIBLES] Up to " << L1_MAX_SHARDS_TO_SPAWN << " shards can spawn (need " << L1_REQUIRED_SHARDS << " to activate portal)" << std::endl;
    std::cout << "  [PORTAL] Portal spawns when " << L1_REQUIRED_SHARDS << " shards collected!" << std::endl;
    std::cout << "  [HUD] Level 1 simple 2D HUD initialized" << std::endl;
}

void Level1OuterDriftZone::setupPlane() {
    // IMPROVED: Create a GRID FLOOR instead of a flat plane
    // This gives CLEAR VISUAL REFERENCE for movement
    // - Grid lines make parallax motion obvious
    // - Multiple colored stripes help track speed
    
    std::vector<float> vertices;
    
    // Create a 50x200 grid with 10x10 unit cells
    // This means 10 columns wide and 40 rows deep
    const int gridWidth = 10;   // Number of cells in X direction
    const int gridDepth = 40;   // Number of cells in Z direction
    const float cellSize = 10.0f;
    
    // Total dimensions: 100 units wide (X), 400 units deep (Z)
    float planeWidth = 50.0f;
    float planeLength = 200.0f;
    
    const float startX = -planeWidth;
    const float startZ = -planeLength;
    
    // Build grid of quads - each cell alternates color via texture coords
    for (int row = 0; row < gridDepth; row++) {
        for (int col = 0; col < gridWidth; col++) {
            float x0 = startX + col * cellSize;
            float x1 = x0 + cellSize;
            float z0 = startZ + row * cellSize;
            float z1 = z0 + cellSize;
            
            // Alternate texture coordinates to create checkerboard pattern
            float uOffset = ((row + col) % 2 == 0) ? 0.0f : 0.5f;
            float vOffset = uOffset;
            
            // Two triangles per cell (positions, normals, texcoords)
            // Triangle 1
            vertices.insert(vertices.end(), {
                x0, 0.0f, z0,  0.0f, 1.0f, 0.0f,  uOffset + 0.0f, vOffset + 0.0f,
                x1, 0.0f, z0,  0.0f, 1.0f, 0.0f,  uOffset + 1.0f, vOffset + 0.0f,
                x1, 0.0f, z1,  0.0f, 1.0f, 0.0f,  uOffset + 1.0f, vOffset + 1.0f
            });
            
            // Triangle 2
            vertices.insert(vertices.end(), {
                x1, 0.0f, z1,  0.0f, 1.0f, 0.0f,  uOffset + 1.0f, vOffset + 1.0f,
                x0, 0.0f, z1,  0.0f, 1.0f, 0.0f,  uOffset + 0.0f, vOffset + 1.0f,
                x0, 0.0f, z0,  0.0f, 1.0f, 0.0f,  uOffset + 0.0f, vOffset + 0.0f
            });
        }
    }
    
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    
    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    planeVertexCount = vertices.size() / 8;  // Store for rendering
    std::cout << "  Grid floor created: " << gridWidth << "x" << gridDepth 
              << " cells (" << planeVertexCount << " vertices)\n";
    
    // Load texture (will use placeholder if not found)
    planeTexture = loadTexture("textures/level1_drift.png");
}

void Level1OuterDriftZone::initAsteroidSpawner() {
    /*
     * ═══════════════════════════════════════════════════════════════════════
     *  LEVEL 1: INFINITE LARGE ASTEROID SPAWN SYSTEM
     * ═══════════════════════════════════════════════════════════════════════
     * 
     * Instead of fixed positions, asteroids spawn continuously ahead of the player.
     * When an asteroid passes behind the player, it's recycled and repositioned ahead.
     * 
     * This creates an endless stream with no gaps.
     */
    
    std::cout << "  [SPAWN] Initializing infinite asteroid spawner..." << std::endl;
    asteroids.clear();
    asteroids.reserve(L1_MAX_LARGE_ASTEROIDS);
    
    // Random number generator for spawn positions
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(L1_PLAY_AREA_MIN_X - 20.0f, L1_PLAY_AREA_MAX_X + 20.0f);
    std::uniform_real_distribution<float> yDist(L1_PLAY_AREA_MIN_Y - 10.0f, L1_PLAY_AREA_MAX_Y + 10.0f);
    std::uniform_real_distribution<float> zOffsetDist(0.0f, 150.0f);  // Spread across 150 units
    std::uniform_real_distribution<float> velXDist(-2.0f, 2.0f);
    std::uniform_real_distribution<float> velYDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> sizeDist(2.4f, 3.5f);
    
    // Spawn initial wave of asteroids spread ahead
    for (int i = 0; i < L1_MAX_LARGE_ASTEROIDS; ++i) {
        float x = xDist(gen);
        float y = yDist(gen);
        float z = -30.0f - zOffsetDist(gen);  // Start at -30 to -180
        
        glm::vec3 pos(x, y, z);
        
        // Slow lateral drift velocity
        glm::vec3 vel(velXDist(gen), velYDist(gen), 0.0f);
        
        float size = sizeDist(gen);
        
        asteroids.emplace_back(pos, vel, size);
    }
    
    std::cout << "  Created " << asteroids.size() << " large asteroids (infinite spawn)" << std::endl;
    asteroidTexture = loadTexture("Asteroid/Asteroid1e_Color_2K.png");
}

void Level1OuterDriftZone::setupAsteroids() {
    // LEGACY FUNCTION - Now replaced by initAsteroidSpawner()
    // This function is no longer used - all setup is done in initAsteroidSpawner()
}

void Level1OuterDriftZone::initSmallAsteroidSpawner() {
    /*
     * ═══════════════════════════════════════════════════════════════════════
     *  LEVEL 1: INFINITE SMALL ASTEROID SPAWN SYSTEM
     * ═══════════════════════════════════════════════════════════════════════
     * 
     * Small asteroids spawn continuously with faster, more diagonal velocities.
     * They're recycled when they pass behind the player.
     */
    
    std::cout << "  [SPAWN] Initializing infinite small asteroid spawner..." << std::endl;
    smallAsteroids.clear();
    smallAsteroids.reserve(L1_MAX_SMALL_ASTEROIDS);
    
    // Random number generator for spawn positions
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(L1_PLAY_AREA_MIN_X - 15.0f, L1_PLAY_AREA_MAX_X + 15.0f);
    std::uniform_real_distribution<float> yDist(L1_PLAY_AREA_MIN_Y - 8.0f, L1_PLAY_AREA_MAX_Y + 8.0f);
    std::uniform_real_distribution<float> zOffsetDist(0.0f, 140.0f);  // Spread across 140 units
    std::uniform_real_distribution<float> velXDist(-5.0f, 5.0f);
    std::uniform_real_distribution<float> velYDist(-3.0f, 3.0f);
    std::uniform_real_distribution<float> velZDist(3.0f, 6.0f);  // Extra forward velocity
    std::uniform_real_distribution<float> sizeDist(0.75f, 0.95f);
    
    // Spawn initial wave of small asteroids
    for (int i = 0; i < L1_MAX_SMALL_ASTEROIDS; ++i) {
        float x = xDist(gen);
        float y = yDist(gen);
        float z = -40.0f - zOffsetDist(gen);  // Start at -40 to -180 (no instant hits)
        
        glm::vec3 pos(x, y, z);
        
        // Faster diagonal movement
        glm::vec3 vel(velXDist(gen), velYDist(gen), velZDist(gen));
        
        float size = sizeDist(gen);
        
        smallAsteroids.emplace_back(pos, vel, size);
    }
    
    std::cout << "  Created " << smallAsteroids.size() << " small asteroids (infinite spawn)" << std::endl;
    smallAsteroidTexture = loadTexture("Breakabel_Asteroids/Albedo.jpg");
    bulletTexture = loadTexture("textures/bullet.png");
}

void Level1OuterDriftZone::setupSmallAsteroids() {
    // LEGACY FUNCTION - Now replaced by initSmallAsteroidSpawner()
    // This function is no longer used
}

void Level1OuterDriftZone::initCollectibleSpawner() {
    /*
     * ═══════════════════════════════════════════════════════════════════════
     *  LEVEL 1: DISTANCE-BASED COLLECTIBLE SPAWN SYSTEM
     * ═══════════════════════════════════════════════════════════════════════
     * 
     * Collectibles spawn procedurally as the player moves forward.
     * - 5 Energy Shards total (spawn over distance)
     * - 3 Power-ups total (interspersed)
     * - No fixed positions, spawned based on distance traveled
     */
    
    std::cout << "  [SPAWN] Initializing distance-based collectible spawner..." << std::endl;
    collectibles.clear();
    
    // Reset spawn counters
    spawnedShards = 0;
    spawnedPowerups = 0;
    distanceTraveled = 0.0f;
    nextCollectibleSpawnDistance = 40.0f;  // First spawn after 40 units
    
    std::cout << "  Collectible spawner ready:" << std::endl;
    std::cout << "    - Up to " << L1_MAX_SHARDS_TO_SPAWN << " Energy Shards can spawn (" << L1_REQUIRED_SHARDS << " required)" << std::endl;
    std::cout << "    - " << L1_MAX_POWERUPS_TO_SPAWN << " Power-ups (spawn over distance)" << std::endl;
    
    // DO NOT create portal yet - it spawns when all shards collected
    portal = nullptr;
    
    // Load textures
    collectibleTexture = loadTexture("textures/collectible.png");
    portalTexture = loadTexture("textures/portal.png");
}

void Level1OuterDriftZone::setupCollectibles() {
    // LEGACY FUNCTION - Now replaced by initCollectibleSpawner()
    // This function is no longer used
}

void Level1OuterDriftZone::updateAsteroidSpawning(float cameraZ) {
    /*
     * INFINITE ASTEROID SPAWN SYSTEM - Recycle asteroids behind player
     * 
     * When an asteroid passes behind the player (z > cameraZ + 20), recycle it
     * by repositioning it ahead of the player at a random position.
     */
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(L1_PLAY_AREA_MIN_X - 20.0f, L1_PLAY_AREA_MAX_X + 20.0f);
    std::uniform_real_distribution<float> yDist(L1_PLAY_AREA_MIN_Y - 10.0f, L1_PLAY_AREA_MAX_Y + 10.0f);
    std::uniform_real_distribution<float> zOffsetDist(0.0f, 30.0f);  // Spawn within 30 units ahead
    std::uniform_real_distribution<float> velXDist(-2.0f, 2.0f);
    std::uniform_real_distribution<float> velYDist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> sizeDist(2.4f, 3.5f);
    
    for (auto& asteroid : asteroids) {
        // If asteroid has passed behind player, recycle it ahead
        if (asteroid.position.z > cameraZ + 20.0f) {
            // Reposition ahead of camera
            asteroid.position.x = xDist(gen);
            asteroid.position.y = yDist(gen);
            asteroid.position.z = cameraZ - L1_SPAWN_DISTANCE_AHEAD - zOffsetDist(gen);
            
            // Asteroids don't have a public velocity setter, so we just reposition them
            // Their internal drift will continue from their current state
        }
    }
    
    // If we somehow have fewer than max, add more
    while (asteroids.size() < L1_MAX_LARGE_ASTEROIDS) {
        float x = xDist(gen);
        float y = yDist(gen);
        float z = cameraZ - L1_SPAWN_DISTANCE_AHEAD - zOffsetDist(gen);
        glm::vec3 pos(x, y, z);
        glm::vec3 vel(velXDist(gen), velYDist(gen), 0.0f);
        float size = sizeDist(gen);
        asteroids.emplace_back(pos, vel, size);
    }
}

void Level1OuterDriftZone::updateSmallAsteroidSpawning(float cameraZ) {
    /*
     * INFINITE SMALL ASTEROID SPAWN SYSTEM - Recycle small asteroids
     */
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(L1_PLAY_AREA_MIN_X - 15.0f, L1_PLAY_AREA_MAX_X + 15.0f);
    std::uniform_real_distribution<float> yDist(L1_PLAY_AREA_MIN_Y - 8.0f, L1_PLAY_AREA_MAX_Y + 8.0f);
    std::uniform_real_distribution<float> zOffsetDist(0.0f, 25.0f);
    std::uniform_real_distribution<float> velXDist(-5.0f, 5.0f);
    std::uniform_real_distribution<float> velYDist(-3.0f, 3.0f);
    std::uniform_real_distribution<float> velZDist(3.0f, 6.0f);
    std::uniform_real_distribution<float> sizeDist(0.75f, 0.95f);
    
    for (auto& smallAst : smallAsteroids) {
        // Instantly recycle breaking or dead asteroids (no death animation delay)
        if (smallAst.getState() == SmallAsteroid::State::Breaking || 
            smallAst.getState() == SmallAsteroid::State::Dead) {
            // Respawn as a new asteroid immediately
            glm::vec3 newPos(xDist(gen), yDist(gen), cameraZ - L1_SPAWN_DISTANCE_AHEAD - zOffsetDist(gen));
            glm::vec3 newVel(velXDist(gen), velYDist(gen), velZDist(gen));
            smallAst.respawn(newPos, newVel, sizeDist(gen));
            continue;
        }
        
        // Only process Normal asteroids below this point
        if (smallAst.getState() != SmallAsteroid::State::Normal) continue;
        
        // If small asteroid has passed behind player, recycle it ahead
        if (smallAst.position.z > cameraZ + 20.0f) {
            // Reposition ahead (position is public)
            smallAst.position.x = xDist(gen);
            smallAst.position.y = yDist(gen);
            smallAst.position.z = cameraZ - L1_SPAWN_DISTANCE_AHEAD - zOffsetDist(gen);
            
            // Reset velocity for variety
            glm::vec3 newVel(velXDist(gen), velYDist(gen), velZDist(gen));
            smallAst.setVelocity(newVel);
        }
    }
    
    // Count only normal (non-breaking, non-dead) asteroids
    int normalCount = 0;
    for (const auto& smallAst : smallAsteroids) {
        if (smallAst.getState() == SmallAsteroid::State::Normal) {
            normalCount++;
        }
    }
    
    // Add more if below max
    while (normalCount < L1_MAX_SMALL_ASTEROIDS) {
        float x = xDist(gen);
        float y = yDist(gen);
        float z = cameraZ - L1_SPAWN_DISTANCE_AHEAD - zOffsetDist(gen);
        glm::vec3 pos(x, y, z);
        glm::vec3 vel(velXDist(gen), velYDist(gen), velZDist(gen));
        float size = sizeDist(gen);
        smallAsteroids.emplace_back(pos, vel, size);
        normalCount++;
    }
}

void Level1OuterDriftZone::updateCollectibleSpawning() {
    /*
     * DISTANCE-BASED COLLECTIBLE SPAWN SYSTEM with RECYCLING
     * 
     * - Respawn missed collectibles (passed behind camera) at new positions ahead
     * - Spawn shards until player has collected enough for portal
     * - Spawn power-ups occasionally (30% when shards needed, 50% after)
     */
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(L1_PLAY_AREA_MIN_X + 10.0f, L1_PLAY_AREA_MAX_X - 10.0f);
    std::uniform_real_distribution<float> yDist(L1_PLAY_AREA_MIN_Y + 5.0f, L1_PLAY_AREA_MAX_Y - 5.0f);
    std::uniform_real_distribution<float> nextSpawnDist(55.0f, 80.0f);  // Longer intervals between spawns
    std::uniform_int_distribution<int> powerupTypeDist(0, 1);  // 0 = JetBooster, 1 = MiniBurst
    std::uniform_real_distribution<float> typeDist(0.0f, 1.0f);
    
    // Respawn missed collectibles (passed behind camera without being collected)
    for (auto& collectible : collectibles) {
        if (!collectible.isCollected() && collectible.getPosition().z > 30.0f) {
            // This collectible was missed - respawn it ahead
            glm::vec3 newPos(xDist(gen), yDist(gen), -L1_SPAWN_DISTANCE_AHEAD * 0.8f);
            collectible.respawn(newPos);
            std::cout << "[RECYCLE] Collectible respawned ahead at z=" << newPos.z << std::endl;
        }
    }
    
    // Check if it's time to spawn a new collectible
    if (distanceTraveled < nextCollectibleSpawnDistance) return;
    
    // Determine what to spawn
    bool needMoreShards = (collectedShardCount < requiredShardCount);
    
    glm::vec3 spawnPos;
    spawnPos.x = xDist(gen);
    spawnPos.y = yDist(gen);
    spawnPos.z = -L1_SPAWN_DISTANCE_AHEAD * 0.8f;  // Spawn ahead
    
    bool spawned = false;
    float spawnRoll = typeDist(gen);
    
    // When shards needed: 70% shard, 30% power-up
    // When all shards collected: 50% power-up, 50% nothing
    if (needMoreShards) {
        if (spawnRoll < 0.7f) {
            // Spawn energy shard (70%)
            collectibles.emplace_back(spawnPos, CollectibleType::EnergyShard);
            collectibles.back().setupMesh();
            spawnedShards++;
            spawned = true;
            std::cout << "[SPAWN] Energy Shard spawned at z=" << spawnPos.z 
                      << " (collected " << collectedShardCount << "/" << L1_REQUIRED_SHARDS << " for portal)" << std::endl;
        } else {
            // Spawn power-up (30%)
            CollectibleType powerupType = (powerupTypeDist(gen) == 0) 
                ? CollectibleType::JetBooster 
                : CollectibleType::MiniBurstShot;
            
            collectibles.emplace_back(spawnPos, powerupType);
            collectibles.back().setupMesh();
            spawnedPowerups++;
            spawned = true;
            
            std::string powerupName = (powerupType == CollectibleType::JetBooster) 
                ? "Jet Booster" : "Mini Burst Shot";
            std::cout << "[SPAWN] " << powerupName << " power-up spawned at z=" << spawnPos.z << std::endl;
        }
    } else {
        // All shards collected - 50% chance for power-up
        if (spawnRoll < 0.5f) {
            CollectibleType powerupType = (powerupTypeDist(gen) == 0) 
                ? CollectibleType::JetBooster 
                : CollectibleType::MiniBurstShot;
            
            collectibles.emplace_back(spawnPos, powerupType);
            collectibles.back().setupMesh();
            spawnedPowerups++;
            spawned = true;
            
            std::string powerupName = (powerupType == CollectibleType::JetBooster) 
                ? "Jet Booster" : "Mini Burst Shot";
            std::cout << "[SPAWN] " << powerupName << " power-up spawned at z=" << spawnPos.z << std::endl;
        }
    }
    
    // Schedule next spawn
    if (spawned) {
        nextCollectibleSpawnDistance = distanceTraveled + nextSpawnDist(gen);
    }
}

void Level1OuterDriftZone::update(float dt, Game& game) {
    /*
     * ON-RAILS SCROLLING UPDATE
     * 
     * In classic arcade style, the world moves TOWARD the player, not the player forward.
     * We update worldZOffset and apply it to all objects' Z positions.
     * Asteroids also have their own velocity for lateral movement.
     */
    
    // Check for Game Over state (ship destroyed)
    PlayerShip* ship = game.getPlayerShip();
    if (ship && !ship->getIsAlive() && !gameOver) {
        gameOver = true;
        game.playGameOverSound();  // Play game over sound
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════╗\n";
        std::cout << "║          GAME OVER                     ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        std::cout << "\n";
    }
    
    // Handle restart on R key press (works on game over OR win)
    if ((gameOver || completed) && glfwGetKey(game.getWindow(), GLFW_KEY_R) == GLFW_PRESS) {
        resetLevel(game);
        std::cout << "[LEVEL 1] Restarting..." << std::endl;
        return;
    }
    
    // When Game Over, stop advancing world scroll and most game logic
    if (gameOver) {
        // Let ship drift to a stop (handled in PlayerShip::update)
        // Update HUD to show final state
        updateHUD(dt, game);
        return;
    }
    
    // Update level timer
    levelTimer += dt;
    lastFireTime += dt;
    
    // Check for slow-down (Left Shift reduces scroll speed temporarily)
    float currentScrollSpeed = scrollSpeed;
    if (glfwGetKey(game.getWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        currentScrollSpeed = scrollSpeed * 0.4f;  // 40% speed when holding Shift
    }
    
    // Apply forward speed boost when jet booster burst is active
    if (ship && ship->isJetBoosterBurstActive()) {
        currentScrollSpeed *= 2.0f;  // Double forward speed during boost
    }
    
    // Track distance traveled for collectible spawning
    distanceTraveled += currentScrollSpeed * dt;
    
    // ON-RAILS: Move the world toward the player (scroll along +Z)
    worldZOffset += currentScrollSpeed * dt;
    
    // Get camera Z position for spawn calculations
    float cameraZ = ship ? ship->getPosition().z : -10.0f;
    
    // INFINITE SPAWN: Recycle and spawn asteroids
    updateAsteroidSpawning(cameraZ);
    updateSmallAsteroidSpawning(cameraZ);
    
    // DISTANCE-BASED: Spawn collectibles as player progresses
    updateCollectibleSpawning();
    
    // Update all large asteroids (drift + scroll toward player)
    // Update large asteroids: drift motion + scroll toward player
    for (auto& asteroid : asteroids) {
        if (asteroid.isDestroyed()) continue;  // Skip destroyed asteroids
        asteroid.update(dt);
        // Apply Level 1 scroll: asteroids move toward player at current speed
        asteroid.position.z += currentScrollSpeed * dt;
    }
    
    // Update small asteroids: rotation + breaking animation + scroll
    // DON'T erase dead asteroids - let spawn system recycle them
    for (auto& smallAst : smallAsteroids) {
        smallAst.update(dt);
        // Apply Level 1 scroll: small asteroids move toward player at current speed
        smallAst.position.z += currentScrollSpeed * dt;
    }
    
    // Update bullets: they move in world space, NOT scrolled
    // Bullets travel at 50 units/sec in -Z direction (their own velocity)
    // They do NOT get L1_SCROLL_SPEED applied - they're independent projectiles
    for (auto& bullet : bullets) {
        bullet.update(dt);
    }
    
    // Remove expired bullets (safe iteration pattern)
    for (size_t i = 0; i < bullets.size(); ) {
        if (bullets[i].isExpired()) {
            bullets.erase(bullets.begin() + i);
        } else {
            ++i;
        }
    }
    
    // Update fragments: fly outward + scroll toward player
    for (auto& frag : fragments) {
        frag.update(dt);
        // Apply Level 1 scroll: fragments move toward player at current speed
        frag.position.z += currentScrollSpeed * dt;
    }
    
    // Remove dead fragments
    fragments.erase(
        std::remove_if(fragments.begin(), fragments.end(),
            [](const AsteroidFragment& f) { return f.isDead(); }),
        fragments.end()
    );
    
    // Update collectibles: bob animation + scroll toward player
    for (auto& collectible : collectibles) {
        collectible.update(dt);
        // Apply Level 1 scroll: collectibles move toward player at current speed
        collectible.position.z += currentScrollSpeed * dt;
    }
    
    // Remove collected collectibles (after pickup animation finishes)
    collectibles.erase(
        std::remove_if(collectibles.begin(), collectibles.end(),
            [](const Collectible& c) { return c.isReadyToRemove(); }),
        collectibles.end()
    );
    
    // Update Wave Portal: spin animation + scroll toward player
    if (portal) {
        portal->update(dt);
        // Apply Level 1 scroll: portal moves toward player at current speed
        portal->position.z += currentScrollSpeed * dt;
    }
    
    // Update power-up timers
    if (jetBoosterActive && levelTimer >= jetBoosterEndTime) {
        jetBoosterActive = false;
        std::cout << "[POWERUP] Jet Booster expired" << std::endl;
        // Update HUD state
        hudState.jetBoosterActive = false;
        hudState.jetBoosterTimeRemaining = 0.0f;
    }
    if (miniBurstActive && levelTimer >= miniBurstEndTime) {
        miniBurstActive = false;
        std::cout << "[POWERUP] Mini Burst Shot expired" << std::endl;
        // Update HUD state
        hudState.miniBurstActive = false;
        hudState.miniBurstTimeRemaining = 0.0f;
    }
    
    // Update player ship power-up states
    if (ship) {
        ship->setJetBoosterActive(jetBoosterActive);
    }
    
    // Check collisions between player ship and large asteroids
    checkCollisions(game);
    
    // Check bullet vs small asteroid collisions
    checkBulletCollisions();
    
    // Check player vs collectible collisions
    checkCollectibleCollisions(game);
    
    // Check asteroid-asteroid collisions (bounce off each other)
    checkAsteroidCollisions();
    
    // Update HUD state and visual feedback timers
    updateHUD(dt, game);
    
    // Portal interaction and capture sequence
    if (portal) {
        PortalState portalState = portal->getState();
        
        // Update portal state in HUD
        hudState.portalActive = portal->isActive();
        
        if (portalBeingCaptured) {
            // Player is being sucked in - move toward portal
            if (ship) {
                glm::vec3 newPos = portal->pullPlayerToward(ship->getPosition(), dt);
                ship->setPosition(newPos);
                
                // Check if capture complete
                if (portalState == PortalState::Completed) {
                    completed = true;
                    std::cout << "[LEVEL COMPLETE] Portal sequence finished! Proceeding to Level 2..." << std::endl;
                }
            }
        } else if (portalState == PortalState::Active) {
            // Portal active - check for entry
            if (ship && portal->checkPlayerEntry(ship->getPosition(), 1.0f)) {
                // Player entered! Start capture sequence
                portal->startCapture();
                portalBeingCaptured = true;
                std::cout << "[PORTAL] 🌀 Entering portal - capture sequence started!" << std::endl;
            }
        }
    }
    
    // Manual completion for testing (ENTER key)
    if (glfwGetKey(game.getWindow(), GLFW_KEY_ENTER) == GLFW_PRESS && !completed) {
        completed = true;
        std::cout << "Level 1 Complete! Press ENTER again to proceed to Level 2..." << std::endl;
    }
}

void Level1OuterDriftZone::checkCollisions(Game& game) {
    PlayerShip* ship = game.getPlayerShip();
    if (!ship || !ship->getIsAlive()) return;  // Don't check collisions if ship is destroyed
    
    // Get player ship position and radius for collision detection
    glm::vec3 shipPos = ship->getPosition();
    float shipRadius = 1.0f;  // Ship collision radius (approximate)
    
    // Check collision with each large asteroid
    for (auto& asteroid : asteroids) {
        // Skip destroyed asteroids
        if (asteroid.isDestroyed()) continue;
        
        if (asteroid.checkCollision(shipPos, shipRadius)) {
            // COLLISION DETECTED!
            std::cout << "[COLLISION] Ship hit large asteroid - asteroid destroyed!" << std::endl;
            
            // Play crash sound
            game.playCrashSound();
            
            // DESTROY the asteroid
            asteroid.destroy();
            
            // Apply damage to ship (triggers invulnerability + flash)
            ship->takeDamage(1);
            
            // Push ship backward strongly
            glm::vec3 shipForward = ship->getForward();
            glm::vec3 pushback = -shipForward * 8.0f;  // Push back 8 units (stronger)
            
            glm::vec3 newPos = shipPos + pushback;
            ship->setPosition(newPos);
            
            // Check if ship was destroyed
            if (!ship->getIsAlive()) {
                gameOver = true;
                game.playGameOverSound();  // Play game over sound
            }
            
            break;  // Only handle one collision per frame
        }
    }
    
    // Check collision with small asteroids - destroys them but damages ship
    for (size_t i = 0; i < smallAsteroids.size(); ++i) {
        if (smallAsteroids[i].isDead() || smallAsteroids[i].getState() == SmallAsteroid::State::Breaking) continue;
        
        if (smallAsteroids[i].checkCollision(shipPos, shipRadius)) {
            // COLLISION DETECTED with small asteroid!
            std::cout << "[COLLISION] Ship hit small asteroid - asteroid destroyed!" << std::endl;
            
            // Play crash sound
            game.playCrashSound();
            
            // Destroy the small asteroid
            smallAsteroids[i].startBreaking();
            
            // Apply damage to ship
            ship->takeDamage(1);
            
            // Check if ship was destroyed
            if (!ship->getIsAlive()) {
                gameOver = true;
                game.playGameOverSound();  // Play game over sound
            }
            
            break;  // Only handle one collision per frame
        }
    }
}

void Level1OuterDriftZone::checkBulletCollisions() {
    // Continuous collision detection - swept sphere for bullets
    for (size_t b = 0; b < bullets.size(); ++b) {
        auto& bullet = bullets[b];
        if (bullet.isExpired()) continue;
        
        bool bulletHit = false;
        glm::vec3 bulletStart = bullet.getPreviousPosition();
        glm::vec3 bulletEnd = bullet.getPosition();
        float bulletRadius = bullet.getRadius();
        
        // Check against large asteroids (use swept collision)
        for (size_t a = 0; a < asteroids.size(); ++a) {
            glm::vec3 astPos = asteroids[a].getPosition();
            // Use 1.3x radius for bullet collision to ensure hits register
            float astRadius = asteroids[a].getRadius() * 1.3f;
            
            // Swept sphere-sphere collision
            if (checkSweptCollision(bulletStart, bulletEnd, bulletRadius, astPos, astRadius)) {
                // Hit large asteroid - just destroy bullet (large asteroids are indestructible)
                bullet.markForDestruction();
                bulletHit = true;
                break;
            }
        }
        
        if (bulletHit) continue;
        
        // Check against small asteroids (use swept collision)
        for (size_t a = 0; a < smallAsteroids.size(); ++a) {
            if (smallAsteroids[a].getState() != SmallAsteroid::State::Normal) continue;
            
            glm::vec3 astPos = smallAsteroids[a].getPosition();
            float astRadius = smallAsteroids[a].getRadius();  // Radius already scaled to match visual
            
            if (checkSweptCollision(bulletStart, bulletEnd, bulletRadius, astPos, astRadius)) {
                // HIT! Start the breaking animation
                std::cout << "[HIT] Bullet destroyed small asteroid at (" 
                          << smallAsteroids[a].getPosition().x << ", "
                          << smallAsteroids[a].getPosition().y << ", "
                          << smallAsteroids[a].getPosition().z << ")" << std::endl;
                
                // Create fragments flying outward from asteroid BEFORE marking as breaking
                glm::vec3 astPosSnap = smallAsteroids[a].getPosition();
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_real_distribution<float> speedDist(3.0f, 8.0f);
                std::uniform_real_distribution<float> angleDist(0.0f, 360.0f);
                
                int numFragments = 2 + (rand() % 2);  // 2-3 fragments
                
                for (int i = 0; i < numFragments; ++i) {
                    float angle = angleDist(gen);
                    float speed = speedDist(gen);
                    glm::vec3 fragVel(
                        cos(glm::radians(angle)) * speed,
                        (rand() % 100 - 50) / 50.0f * 2.0f,  // Random Y velocity
                        sin(glm::radians(angle)) * speed
                    );
                    
                    fragments.emplace_back(astPosSnap, fragVel, 0.4f);
                }
                
                // Now mark asteroid as breaking
                smallAsteroids[a].startBreaking();
                
                // Add score for destroying small asteroid
                score += 5;
                
                // Play sound effect (placeholder)
                playAsteroidBreakSound();
                
                // Mark bullet for destruction (safe removal)
                bullet.markForDestruction();
                break;  // Bullet can only hit one asteroid
            }
        }
    }
}

// Helper function for swept sphere-sphere collision
bool Level1OuterDriftZone::checkSweptCollision(const glm::vec3& start, const glm::vec3& end, 
                                                float radius1, const glm::vec3& targetPos, float radius2) {
    glm::vec3 ray = end - start;
    float rayLength = glm::length(ray);
    
    if (rayLength < 0.0001f) {
        // No movement, use simple sphere test
        float dist = glm::length(start - targetPos);
        return dist < (radius1 + radius2);
    }
    
    glm::vec3 rayDir = ray / rayLength;
    glm::vec3 toTarget = targetPos - start;
    float projection = glm::dot(toTarget, rayDir);
    projection = glm::clamp(projection, 0.0f, rayLength);
    glm::vec3 closestPoint = start + rayDir * projection;
    float distanceToCenter = glm::length(closestPoint - targetPos);
    
    return distanceToCenter < (radius1 + radius2);
}

void Level1OuterDriftZone::checkAsteroidCollisions() {
    const float restitution = 0.85f;  // Bounciness (0.85 = slightly damped elastic collision)
    
    // Large asteroid vs large asteroid
    for (size_t i = 0; i < asteroids.size(); ++i) {
        for (size_t j = i + 1; j < asteroids.size(); ++j) {
            glm::vec3 posA = asteroids[i].getPosition();
            glm::vec3 posB = asteroids[j].getPosition();
            // Collision radius = visual scale (radius * 1.2)
            float radiusA = asteroids[i].getRadius() * 1.2f;
            float radiusB = asteroids[j].getRadius() * 1.2f;
            
            float dist = glm::length(posB - posA);
            float minDist = radiusA + radiusB;
            
            // Detect collision when they touch
            if (dist < minDist && dist > 0.0001f) {
                glm::vec3 normal = glm::normalize(posB - posA);
                
                // Compute overlap and separate asteroids
                float overlap = minDist - dist;
                asteroids[i].position -= normal * (overlap * 0.5f);
                asteroids[j].position += normal * (overlap * 0.5f);
                
                // Apply bounce velocity (elastic collision, equal mass)
                glm::vec3 velA = asteroids[i].getVelocity();
                glm::vec3 velB = asteroids[j].getVelocity();
                glm::vec3 relVel = velA - velB;
                float velAlongNormal = glm::dot(relVel, normal);
                
                if (velAlongNormal < 0.0f) {
                    float impulseStrength = -(1.0f + restitution) * velAlongNormal / 2.0f;
                    glm::vec3 impulse = impulseStrength * normal;
                    asteroids[i].setVelocity(velA + impulse);
                    asteroids[j].setVelocity(velB - impulse);
                }
            }
        }
    }
    
    // Small asteroid vs small asteroid
    for (size_t i = 0; i < smallAsteroids.size(); ++i) {
        if (smallAsteroids[i].isDead() || smallAsteroids[i].getState() == SmallAsteroid::State::Breaking) continue;
        
        for (size_t j = i + 1; j < smallAsteroids.size(); ++j) {
            if (smallAsteroids[j].isDead() || smallAsteroids[j].getState() == SmallAsteroid::State::Breaking) continue;
            
            glm::vec3 posA = smallAsteroids[i].getPosition();
            glm::vec3 posB = smallAsteroids[j].getPosition();
            // Small asteroids have scale 1.0, so radius = radius (no multiplier)
            float radiusA = smallAsteroids[i].getRadius();
            float radiusB = smallAsteroids[j].getRadius();
            
            float dist = glm::length(posB - posA);
            float minDist = radiusA + radiusB;
            
            if (dist < minDist && dist > 0.0001f) {
                glm::vec3 normal = glm::normalize(posB - posA);
                float overlap = minDist - dist;
                smallAsteroids[i].position -= normal * (overlap * 0.5f);
                smallAsteroids[j].position += normal * (overlap * 0.5f);
                
                glm::vec3 velA = smallAsteroids[i].getVelocity();
                glm::vec3 velB = smallAsteroids[j].getVelocity();
                glm::vec3 relVel = velA - velB;
                float velAlongNormal = glm::dot(relVel, normal);
                
                if (velAlongNormal < 0.0f) {
                    float impulseStrength = -(1.0f + restitution) * velAlongNormal / 2.0f;
                    glm::vec3 impulse = impulseStrength * normal;
                    smallAsteroids[i].setVelocity(velA + impulse);
                    smallAsteroids[j].setVelocity(velB - impulse);
                }
            }
        }
    }
    
    // Large asteroid vs small asteroid
    for (size_t i = 0; i < asteroids.size(); ++i) {
        for (size_t j = 0; j < smallAsteroids.size(); ++j) {
            if (smallAsteroids[j].isDead() || smallAsteroids[j].getState() == SmallAsteroid::State::Breaking) continue;
            
            glm::vec3 posLarge = asteroids[i].getPosition();
            glm::vec3 posSmall = smallAsteroids[j].getPosition();
            float radiusLarge = asteroids[i].getRadius() * 1.2f;
            float radiusSmall = smallAsteroids[j].getRadius();
            
            float dist = glm::length(posSmall - posLarge);
            float minDist = radiusLarge + radiusSmall;
            
            if (dist < minDist && dist > 0.0001f) {
                glm::vec3 normal = glm::normalize(posSmall - posLarge);
                float overlap = minDist - dist;
                asteroids[i].position -= normal * (overlap * 0.5f);
                smallAsteroids[j].position += normal * (overlap * 0.5f);
                
                glm::vec3 velLarge = asteroids[i].getVelocity();
                glm::vec3 velSmall = smallAsteroids[j].getVelocity();
                glm::vec3 relVel = velLarge - velSmall;
                float velAlongNormal = glm::dot(relVel, normal);
                
                if (velAlongNormal < 0.0f) {
                    float impulseStrength = -(1.0f + restitution) * velAlongNormal / 2.0f;
                    glm::vec3 impulse = impulseStrength * normal;
                    asteroids[i].setVelocity(velLarge + impulse);
                    smallAsteroids[j].setVelocity(velSmall - impulse);
                }
            }
        }
    }
}

void Level1OuterDriftZone::checkCollectibleCollisions(Game& game) {
    PlayerShip* ship = game.getPlayerShip();
    if (!ship) return;
    
    glm::vec3 shipPos = ship->getPosition();
    float shipRadius = 1.0f;  // Approximate ship collision radius
    
    // Use index-based loop to avoid iterator invalidation issues
    for (size_t i = 0; i < collectibles.size(); ++i) {
        if (collectibles[i].isCollected()) continue;
        
        if (collectibles[i].checkCollision(shipPos, shipRadius)) {
            // Collect it!
            collectibles[i].collect();
            
            // Handle based on type
            switch (collectibles[i].getType()) {
                case CollectibleType::EnergyShard:
                    game.playCollectSound();  // Play collect sound
                    collectedShardCount++;
                    score += 100;  // 100 points per collectible
                    std::cout << "[PROGRESS] Energy Shards: " << collectedShardCount << "/" << L1_REQUIRED_SHARDS << " | Score: " << score << std::endl;
                    
                    // Update HUD state - trigger shard collected flash
                    hudState.collectedShards = collectedShardCount;
                    hudState.shardCollectedFlashTimer = 0.3f;  // Flash for 0.3 seconds
                    
                    // SPAWN portal when required shards collected (extras are bonus)
                    if (collectedShardCount >= L1_REQUIRED_SHARDS && !portal) {
                        // Spawn portal ahead of player (150-200 units in front)
                        float portalZ = shipPos.z - 175.0f;  // 175 units ahead of player
                        portal = new WavePortal(glm::vec3(0.0f, 0.0f, portalZ));
                        portal->setupMesh();
                        portal->spawn();  // Begin materialize animation
                        
                        std::cout << "[PORTAL] ✨ All shards collected! Portal spawning ahead at z=" 
                                  << portalZ << std::endl;
                        
                        // Update HUD state
                        hudState.portalActive = false;  // Not active yet (still spawning)
                        hudState.portalActivatedFlashTimer = 2.0f;  // Flash during spawn
                    }
                    break;
                    
                case CollectibleType::JetBooster:
                    game.playPowerupSound();  // Play powerup sound
                    // Give the player a boost charge instead of timed power-up
                    ship->addJetBoosterCharge();
                    score += 100;  // 100 points per collectible
                    std::cout << "[POWERUP] Jet Booster charge collected! Press SPACE to boost. (" 
                              << ship->getJetBoosterCharges() << " charges) | Score: " << score << std::endl;
                    
                    // Update HUD state - trigger power-up flash
                    hudState.jetBoosterActive = true;
                    hudState.jetBoosterTimeRemaining = static_cast<float>(ship->getJetBoosterCharges());  // Show charges
                    hudState.powerupStartFlashTimer = 0.5f;  // Flash for 0.5 seconds
                    break;
                    
                case CollectibleType::MiniBurstShot:
                    game.playPowerupSound();  // Play powerup sound
                    miniBurstActive = true;
                    miniBurstEndTime = levelTimer + powerupDuration;
                    score += 100;  // 100 points per collectible
                    std::cout << "[POWERUP] Mini Burst Shot active for " << powerupDuration << " seconds! | Score: " << score << std::endl;
                    
                    // Update HUD state - trigger power-up flash
                    hudState.miniBurstActive = true;
                    hudState.miniBurstTimeRemaining = powerupDuration;
                    hudState.powerupStartFlashTimer = 0.5f;  // Flash for 0.5 seconds
                    break;
            }
        }
    }
}

void Level1OuterDriftZone::fireBullet(const glm::vec3& position, const glm::vec3& direction) {
    // Don't allow firing when Game Over
    if (gameOver) {
        return;
    }
    
    // Fire rate is now handled by Game.cpp - no duplicate checking here
    // This function just spawns the bullet(s)
    
    // Mini Burst Shot: Fire 3 bullets in a spread
    if (miniBurstActive) {
        // Center bullet
        glm::vec3 spawnPos = position + direction * 1.2f;
        bullets.emplace_back(spawnPos, direction, 50.0f);
        
        // Left bullet (offset and angle)
        glm::vec3 leftDir = glm::normalize(direction + glm::vec3(-0.15f, 0.0f, 0.0f));
        glm::vec3 leftPos = position + glm::vec3(-0.3f, 0.0f, 0.0f) + leftDir * 1.2f;
        bullets.emplace_back(leftPos, leftDir, 50.0f);
        
        // Right bullet (offset and angle)
        glm::vec3 rightDir = glm::normalize(direction + glm::vec3(0.15f, 0.0f, 0.0f));
        glm::vec3 rightPos = position + glm::vec3(0.3f, 0.0f, 0.0f) + rightDir * 1.2f;
        bullets.emplace_back(rightPos, rightDir, 50.0f);
        
        std::cout << "[FIRE] Mini Burst Shot! 3 bullets fired. Total: " << bullets.size() << std::endl;
    }
    else {
        // Normal single bullet
        glm::vec3 spawnPos = position + direction * 1.2f;
        bullets.emplace_back(spawnPos, direction, 50.0f);
        std::cout << "[FIRE] Bullet fired. Total: " << bullets.size() << std::endl;
    }
}

void Level1OuterDriftZone::playAsteroidBreakSound() {
    // TODO: Implement audio system
    // For now, just a placeholder
    // Later: soundEngine->playSound("asteroid_break.wav");
}

void Level1OuterDriftZone::render(Game& game) {
    // Render skybox first (before everything else)
    if (skybox && game.getCameraController()) {
        // Get projection matrix from screen dimensions
        float aspectRatio = (float)game.getScreenWidth() / (float)game.getScreenHeight();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);
        skybox->render(game.getCameraController()->getViewMatrix(), projection);
    }
    
    Shader* shader = game.getTexturedShader();
    if (!shader) {
        std::cerr << "[ERROR] Shader is null in Level1 render!" << std::endl;
        return;
    }
    shader->use();
    
    // Render the grid floor first (DISABLED - relying on asteroids and space)
    if (L1_RENDER_FLOOR) {
        glm::mat4 planeModel = glm::mat4(1.0f);
        planeModel = glm::translate(planeModel, glm::vec3(0.0f, -2.0f, 0.0f));
        shader->setMat4("model", planeModel);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, planeTexture);
        shader->setInt("texture1", 0);
        
        glBindVertexArray(planeVAO);
        glDrawArrays(GL_TRIANGLES, 0, planeVertexCount);
        glBindVertexArray(0);
    }
    
    // Render all large asteroids
    for (auto& asteroid : asteroids) {
        if (asteroid.isDestroyed()) continue;  // Skip destroyed asteroids
        shader->setMat4("model", asteroid.getModelMatrix());
        asteroid.render(asteroidTexture);
    }
    
    // Render all small asteroids
    for (auto& smallAst : smallAsteroids) {
        shader->setMat4("model", smallAst.getModelMatrix());
        smallAst.render(smallAsteroidTexture);
    }
    
    // Render all bullets - use index-based loop for safety
    for (size_t i = 0; i < bullets.size(); ++i) {
        shader->setMat4("model", bullets[i].getModelMatrix());
        bullets[i].render(bulletTexture);
    }
    
    // Render fragments
    for (size_t i = 0; i < fragments.size(); ++i) {
        shader->setMat4("model", fragments[i].getModelMatrix());
        fragments[i].render(smallAsteroidTexture);  // Reuse small asteroid texture
    }
    
    // Render collectibles - Energy Shards and Power-ups with glow effect
    float glowPulse = (sin(levelTimer * 4.0f) + 1.0f) / 2.0f;  // Pulse 0-1
    shader->setInt("isCollectible", 1);
    shader->setFloat("glowPulse", glowPulse);
    
    for (auto& collectible : collectibles) {
        if (!collectible.isCollected()) {
            shader->setMat4("model", collectible.getModelMatrix());
            
            // Blue glow for energy shards, green for power-ups
            if (collectible.getType() == CollectibleType::EnergyShard) {
                shader->setVec3("collectibleGlow", glm::vec3(0.3f, 0.5f, 1.0f));  // Blue
            } else {
                shader->setVec3("collectibleGlow", glm::vec3(0.3f, 1.0f, 0.3f));  // Green
            }
            
            collectible.render(collectibleTexture);
        }
    }
    shader->setInt("isCollectible", 0);  // Reset
    
    // Render Wave Portal - end goal of Level 1
    if (portal) {
        shader->setMat4("model", portal->getModelMatrix());
        // Set portal shader uniforms for orange color
        shader->setInt("isPortal", 1);
        shader->setVec3("portalColor", glm::vec3(1.0f, 0.5f, 0.0f));  // Orange
        portal->render(portalTexture);
        shader->setInt("isPortal", 0);  // Reset for other objects
    }
    
    // Update HUD (window title display)
    renderHUD(game);
}

bool Level1OuterDriftZone::isCompleted() const {
    return completed;
}

std::string Level1OuterDriftZone::getLevelName() const {
    return "Wave 1 - Outer Drift Zone";
}

LevelType Level1OuterDriftZone::getLevelType() const {
    return LevelType::Level1_OuterDriftZone;
}

void Level1OuterDriftZone::cleanup() {
    // Cleanup skybox
    if (skybox) {
        delete skybox;
        skybox = nullptr;
    }
    
    // Clean up large asteroids
    for (auto& asteroid : asteroids) {
        asteroid.cleanup();
    }
    asteroids.clear();
    
    // Clean up small asteroids
    for (auto& smallAst : smallAsteroids) {
        smallAst.cleanup();
    }
    smallAsteroids.clear();
    
    // Clean up bullets
    for (auto& bullet : bullets) {
        bullet.cleanup();
    }
    bullets.clear();
    
    // Clean up fragments
    for (auto& frag : fragments) {
        frag.cleanup();
    }
    fragments.clear();
    
    // Clean up collectibles
    for (auto& collectible : collectibles) {
        collectible.cleanup();
    }
    collectibles.clear();
    
    // Clean up Wave Portal
    if (portal) {
        portal->cleanup();
        delete portal;
        portal = nullptr;
    }
    
    // Clean up textures
    if (asteroidTexture != 0) {
        glDeleteTextures(1, &asteroidTexture);
        asteroidTexture = 0;
    }
    if (smallAsteroidTexture != 0) {
        glDeleteTextures(1, &smallAsteroidTexture);
        smallAsteroidTexture = 0;
    }
    if (bulletTexture != 0) {
        glDeleteTextures(1, &bulletTexture);
        bulletTexture = 0;
    }
    if (collectibleTexture != 0) {
        glDeleteTextures(1, &collectibleTexture);
        collectibleTexture = 0;
    }
    if (portalTexture != 0) {
        glDeleteTextures(1, &portalTexture);
        portalTexture = 0;
    }
    
    // Clean up plane geometry
    if (planeVAO != 0) {
        glDeleteVertexArrays(1, &planeVAO);
        glDeleteBuffers(1, &planeVBO);
        glDeleteTextures(1, &planeTexture);
        planeVAO = 0;
        planeVBO = 0;
        planeTexture = 0;
    }
    
    // Clean up 2D HUD
    cleanupHUD2D();
}

void Level1OuterDriftZone::resetLevel(Game& game) {
    /*
     * ═══════════════════════════════════════════════════════════════
     *  LEVEL 1 RESET - Clean restart for retry after Game Over
     * ═══════════════════════════════════════════════════════════════
     */
    
    // Clear all dynamic game objects
    bullets.clear();
    fragments.clear();
    
    // Reset level state
    completed = false;
    gameOver = false;
    levelTimer = 0.0f;
    lastFireTime = 0.0f;
    collectedShardCount = 0;
    worldZOffset = 0.0f;
    score = 0;  // Reset score
    
    // Reset infinite spawn state
    distanceTraveled = 0.0f;
    nextCollectibleSpawnDistance = 40.0f;
    spawnedShards = 0;
    spawnedPowerups = 0;
    
    // Reset power-ups
    jetBoosterActive = false;
    jetBoosterEndTime = 0.0f;
    miniBurstActive = false;
    miniBurstEndTime = 0.0f;
    
    // Re-initialize infinite spawn systems
    initAsteroidSpawner();
    initSmallAsteroidSpawner();
    initCollectibleSpawner();
    
    // Reset portal
    if (portal) {
        portal->cleanup();
        delete portal;
        portal = nullptr;  // Don't create until shards collected
    }
    
    // Reset player ship health and position
    game.getPlayerShip()->resetHealth();
    game.getPlayerShip()->setPosition(getPlayerSpawnPosition());
    
    // Reset HUD state
    hudState = Level1HUDState();
    hudState.totalShards = requiredShardCount;
    
    std::cout << "[LEVEL 1] Level reset complete. Good luck!" << std::endl;
}

glm::vec3 Level1OuterDriftZone::getPlayerSpawnPosition() const {
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

glm::vec3 Level1OuterDriftZone::getPlayerSpawnRotation() const {
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

/*
 * ═══════════════════════════════════════════════════════════════════════
 *  LEVEL 1 HUD SYSTEM - Update and Rendering
 * ═══════════════════════════════════════════════════════════════════════
 */

void Level1OuterDriftZone::updateHUD(float dt, Game& game) {
    /*
     * Update HUD state with current game values and tick down flash timers.
     * 
     * This function syncs the HUD state with actual game state each frame,
     * ensuring the display always shows accurate information.
     * 
     * Flash timers provide brief visual feedback when events occur:
     * - Shard collected: Flash shard count for 0.3s
     * - Power-up activated: Flash power-up line for 0.5s
     * - Portal activated: Flash portal status for 1.0s
     */
    
    // Update ship health from PlayerShip
    PlayerShip* ship = game.getPlayerShip();
    if (ship) {
        hudState.shipHealth = ship->getHealth();
        hudState.shipMaxHealth = ship->getMaxHealth();
        hudState.shipInvulnerable = ship->getIsInvulnerable();
        hudState.shipAlive = ship->getIsAlive();
    }
    
    // Update shard progress
    hudState.collectedShards = collectedShardCount;
    hudState.totalShards = requiredShardCount;
    
    // Update power-up states
    // JetBooster: Show charges available or burst active status
    if (ship) {
        if (ship->isJetBoosterBurstActive()) {
            hudState.jetBoosterActive = true;
            hudState.jetBoosterTimeRemaining = 2.0f;  // Show as active
        } else if (ship->getJetBoosterCharges() > 0) {
            hudState.jetBoosterActive = true;
            hudState.jetBoosterTimeRemaining = static_cast<float>(ship->getJetBoosterCharges());
        } else {
            hudState.jetBoosterActive = false;
            hudState.jetBoosterTimeRemaining = 0.0f;
        }
    }
    
    if (miniBurstActive) {
        float timeLeft = miniBurstEndTime - levelTimer;
        hudState.miniBurstTimeRemaining = std::max(0.0f, timeLeft);
    } else {
        hudState.miniBurstTimeRemaining = 0.0f;
    }
    
    // Update portal status
    hudState.portalActive = (portal && portal->isActive());
    
    // Tick down flash timers
    if (hudState.shardCollectedFlashTimer > 0.0f) {
        hudState.shardCollectedFlashTimer -= dt;
    }
    if (hudState.powerupStartFlashTimer > 0.0f) {
        hudState.powerupStartFlashTimer -= dt;
    }
    if (hudState.portalActivatedFlashTimer > 0.0f) {
        hudState.portalActivatedFlashTimer -= dt;
    }
}

void Level1OuterDriftZone::renderHUD(Game& game) {
    /*
     * ═══════════════════════════════════════════════════════════════
     *  SIMPLE 2D HUD - Colored bars in orthographic screen space
     * ═══════════════════════════════════════════════════════════════
     */
    
    // Keep window title for debug
    std::string title = "Starwave 3D | ";
    title += "HP: " + std::to_string(hudState.shipHealth) + "/" + std::to_string(hudState.shipMaxHealth);
    if (hudState.shipInvulnerable) title += " [INVULN]";
    if (!hudState.shipAlive) title += " [DEAD]";
    title += " | Shards: " + std::to_string(hudState.collectedShards) + "/" + std::to_string(hudState.totalShards);
    title += " | Portal: ";
    if (!portal) title += "LOCKED";
    else if (portal->getState() == PortalState::Activating) title += "SPAWNING";
    else title += hudState.portalActive ? "ACTIVE" : "LOCKED";
    glfwSetWindowTitle(game.getWindow(), title.c_str());
    
    // Render simple 2D HUD overlay
    renderHUD2D();
}

// ═══════════════════════════════════════════════════════════════════════
//  2D HUD RENDERING - Simple colored bars
// ═══════════════════════════════════════════════════════════════════════

void Level1OuterDriftZone::setupHUD2D(int screenWidth, int screenHeight) {
    // Create simple quad for HUD elements
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 2, NULL, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    // Create simple shader for colored quads
    const char* vertSrc = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(aPos, 0.0, 1.0);
        }
    )";
    
    const char* fragSrc = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";
    
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertSrc, NULL);
    glCompileShader(vertShader);
    
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragSrc, NULL);
    glCompileShader(fragShader);
    
    hudShaderProgram = glCreateProgram();
    glAttachShader(hudShaderProgram, vertShader);
    glAttachShader(hudShaderProgram, fragShader);
    glLinkProgram(hudShaderProgram);
    
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
}

void Level1OuterDriftZone::cleanupHUD2D() {
    if (hudVAO != 0) {
        glDeleteVertexArrays(1, &hudVAO);
        hudVAO = 0;
    }
    if (hudVBO != 0) {
        glDeleteBuffers(1, &hudVBO);
        hudVBO = 0;
    }
    if (hudShaderProgram != 0) {
        glDeleteProgram(hudShaderProgram);
        hudShaderProgram = 0;
    }
}

void Level1OuterDriftZone::drawHUDQuad(float x, float y, float width, float height, float r, float g, float b) {
    float vertices[] = {
        x, y,
        x + width, y,
        x, y + height,
        x, y + height,
        x + width, y,
        x + width, y + height
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glUseProgram(hudShaderProgram);
    GLint colorLoc = glGetUniformLocation(hudShaderProgram, "color");
    glUniform3f(colorLoc, r, g, b);
    
    glBindVertexArray(hudVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Level1OuterDriftZone::renderHUD2D() {
    if (hudShaderProgram == 0) return;
    
    // Save OpenGL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(hudShaderProgram);
    
    // Use stored screen dimensions for proper centering
    float screenW = static_cast<float>(hudScreenWidth);
    float screenH = static_cast<float>(hudScreenHeight);
    
    // Orthographic projection using actual screen size
    glm::mat4 projection = glm::ortho(0.0f, screenW, screenH, 0.0f);
    GLint projLoc = glGetUniformLocation(hudShaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    
    float margin = 20.0f;
    float barHeight = 30.0f;
    float yPos = margin;
    
    // Health hearts (red) - only draw hearts for remaining health
    float heartSize = 35.0f;
    for (int i = 0; i < hudState.shipHealth; ++i) {
        float r, g, b;
        if (hudState.shipInvulnerable && (int)(levelTimer * 8.0f) % 2 == 0) {
            r = 0.0f; g = 0.9f; b = 1.0f;  // Cyan blink when invulnerable
        } else {
            r = 1.0f; g = 0.1f; b = 0.1f;  // Red heart
        }
        drawHeart(margin + i * (heartSize + 10.0f), yPos, heartSize, r, g, b);
    }
    yPos += heartSize + 15.0f;
    
    // Shard bars (5 segments)
    for (int i = 0; i < hudState.totalShards; ++i) {
        float r, g, b;
        if (i < hudState.collectedShards) {
            r = 1.0f; g = 0.9f; b = 0.0f;  // Gold
        } else {
            r = 0.3f; g = 0.3f; b = 0.3f;  // Dark gray
        }
        drawHUDQuad(margin + i * 35.0f, yPos, 30.0f, barHeight, r, g, b);
    }
    yPos += barHeight + 10.0f;
    
    // Jet Booster power-up bar (when active) - Cyan with name label
    if (hudState.jetBoosterActive && hudState.jetBoosterTimeRemaining > 0.0f) {
        float maxWidth = 150.0f;
        float fillRatio = hudState.jetBoosterTimeRemaining / 2.0f;  // 2 second duration
        float fillWidth = maxWidth * fillRatio;
        
        // Power-up name label above the bar
        winTextRenderer.renderTextScaled("JET BOOSTER", margin, yPos - 2.0f, glm::vec3(0.0f, 0.9f, 1.0f), 1.5f);
        yPos += 18.0f;
        
        // Background bar
        drawHUDQuad(margin, yPos, maxWidth, barHeight, 0.3f, 0.3f, 0.3f);
        // Fill bar (cyan pulse)
        float pulse = 0.8f + 0.2f * sin(levelTimer * 10.0f);
        drawHUDQuad(margin, yPos, fillWidth, barHeight, 0.0f, 0.8f * pulse, 1.0f * pulse);
        yPos += barHeight + 10.0f;
    }
    
    // Mini Burst power-up bar (when active) - Orange with name label
    if (hudState.miniBurstActive && hudState.miniBurstTimeRemaining > 0.0f) {
        float maxWidth = 150.0f;
        float fillRatio = hudState.miniBurstTimeRemaining / 2.0f;  // 2 second duration
        float fillWidth = maxWidth * fillRatio;
        
        // Power-up name label above the bar
        winTextRenderer.renderTextScaled("MINI BURST", margin, yPos - 2.0f, glm::vec3(1.0f, 0.6f, 0.0f), 1.5f);
        yPos += 18.0f;
        
        // Background bar
        drawHUDQuad(margin, yPos, maxWidth, barHeight, 0.3f, 0.3f, 0.3f);
        // Fill bar (orange pulse)
        float pulse = 0.8f + 0.2f * sin(levelTimer * 10.0f);
        drawHUDQuad(margin, yPos, fillWidth, barHeight, 1.0f * pulse, 0.5f * pulse, 0.0f);
        yPos += barHeight + 10.0f;
    }
    
    // Portal status bar
    float r, g, b;
    if (hudState.portalActive) {
        r = 0.0f; g = 1.0f; b = 0.5f;  // Green (active)
    } else if (portal && portal->getState() == PortalState::Activating) {
        r = 1.0f; g = 0.5f; b = 0.0f;  // Orange (spawning)
    } else {
        r = 0.5f; g = 0.5f; b = 0.5f;  // Gray (locked)
    }
    drawHUDQuad(margin, yPos, 150.0f, barHeight, r, g, b);
    
    // Score display (top-right corner)
    std::string scoreText = std::to_string(score);
    float scoreX = 1280.0f - margin - (scoreText.length() * 12.0f * 2.0f);  // Right-aligned
    winTextRenderer.renderTextScaled(scoreText, scoreX, margin, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f);
    
    // Game Over overlay
    if (!hudState.shipAlive) {
        // Calculate centered position for "GAME OVER" (9 chars)
        float gameOverScale = 2.5f;
        float charWidth = 8.0f * gameOverScale;
        float gameOverTextWidth = 9 * charWidth;  // "GAME OVER" = 9 chars
        float gameOverX = (hudScreenWidth - gameOverTextWidth) / 2.0f;
        float gameOverY = hudScreenHeight / 2.0f;
        
        // Red flash overlay - centered
        float overlayWidth = 480.0f;
        float overlayHeight = 120.0f;
        float overlayX = (hudScreenWidth - overlayWidth) / 2.0f;
        float overlayY = gameOverY - overlayHeight / 2.0f - 20.0f;
        drawHUDQuad(overlayX, overlayY, overlayWidth, overlayHeight, 1.0f, 0.0f, 0.0f);
        
        // Show "GAME OVER" text - centered
        winTextRenderer.renderTextScaled("GAME OVER", gameOverX, gameOverY, glm::vec3(1.0f, 1.0f, 1.0f), gameOverScale);
    }
    
    // Win overlay (if completed)
    if (completed) {
        // Calculate centered position for "YOU WIN!!" (9 chars)
        float winScale = 3.0f;
        float winCharWidth = 8.0f * winScale;
        float winTextWidth = 9 * winCharWidth;  // "YOU WIN!!" = 9 chars
        float winX = (hudScreenWidth - winTextWidth) / 2.0f;
        float winY = hudScreenHeight / 2.0f + 20.0f;
        
        // Calculate score text position
        std::string scoreStr = "SCORE:" + std::to_string(score);
        float scoreScale = 2.0f;
        float scoreCharWidth = 8.0f * scoreScale;
        float scoreTextWidth = scoreStr.length() * scoreCharWidth;
        float scoreX = (hudScreenWidth - scoreTextWidth) / 2.0f;
        float scoreY = winY - 60.0f;
        
        // Green overlay - centered
        float overlayWidth = 480.0f;
        float overlayHeight = 160.0f;
        float overlayX = (hudScreenWidth - overlayWidth) / 2.0f;
        float overlayY = winY - overlayHeight / 2.0f - 40.0f;
        drawHUDQuad(overlayX, overlayY, overlayWidth, overlayHeight, 0.0f, 0.8f, 0.2f);
        
        winTextRenderer.renderTextScaled("YOU WIN!!", winX, winY, glm::vec3(1.0f, 1.0f, 1.0f), winScale);
        winTextRenderer.renderTextScaled(scoreStr, scoreX, scoreY, glm::vec3(1.0f, 1.0f, 1.0f), scoreScale);
    }
    
    // Restore OpenGL state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Level1OuterDriftZone::drawHeart(float x, float y, float size, float r, float g, float b) {
    // Draw a heart shape using multiple triangles
    // Heart is composed of two circles on top and a triangle below
    
    float halfSize = size / 2.0f;
    float quarterSize = size / 4.0f;
    
    // Left circle (top-left bump)
    int segments = 12;
    for (int i = 0; i < segments; ++i) {
        float angle1 = 3.14159f + (3.14159f * i / segments);
        float angle2 = 3.14159f + (3.14159f * (i + 1) / segments);
        
        float cx = x + quarterSize;
        float cy = y + quarterSize;
        
        float x1 = cx + cos(angle1) * quarterSize;
        float y1 = cy + sin(angle1) * quarterSize;
        float x2 = cx + cos(angle2) * quarterSize;
        float y2 = cy + sin(angle2) * quarterSize;
        
        // Triangle from center to arc segment
        float vertices[] = {
            cx, cy,
            x1, y1,
            x2, y2
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        
        glUseProgram(hudShaderProgram);
        GLint colorLoc = glGetUniformLocation(hudShaderProgram, "color");
        glUniform3f(colorLoc, r, g, b);
        
        glBindVertexArray(hudVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    
    // Right circle (top-right bump)
    for (int i = 0; i < segments; ++i) {
        float angle1 = 3.14159f + (3.14159f * i / segments);
        float angle2 = 3.14159f + (3.14159f * (i + 1) / segments);
        
        float cx = x + halfSize + quarterSize;
        float cy = y + quarterSize;
        
        float x1 = cx + cos(angle1) * quarterSize;
        float y1 = cy + sin(angle1) * quarterSize;
        float x2 = cx + cos(angle2) * quarterSize;
        float y2 = cy + sin(angle2) * quarterSize;
        
        float vertices[] = {
            cx, cy,
            x1, y1,
            x2, y2
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        
        glBindVertexArray(hudVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    
    // Center rectangle connecting the two bumps
    drawHUDQuad(x + quarterSize, y, halfSize, quarterSize, r, g, b);
    
    // Bottom triangle (the point of the heart)
    float triVertices[] = {
        x, y + quarterSize,
        x + size, y + quarterSize,
        x + halfSize, y + size
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(triVertices), triVertices);
    
    glUseProgram(hudShaderProgram);
    GLint colorLoc = glGetUniformLocation(hudShaderProgram, "color");
    glUniform3f(colorLoc, r, g, b);
    
    glBindVertexArray(hudVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}
