#include "Level2SolarDebrisPath.h"
#include "Game.h"
#include "Texture.h"
#include "Shader.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <random>
#include <algorithm>

/*
 * ═══════════════════════════════════════════════════════════════
 *  LEVEL 2: SOLAR DEBRIS PATH - SCROLL CONFIGURATION
 * ═══════════════════════════════════════════════════════════════
 * 
 * Harsher level with motion-based hazards:
 * - Rotating satellite fragments
 * - Swinging solar panels (pendulums)
 * - Fast breakable comet rocks
 * - Red energy cells to collect
 * - Burst Core power-up for rapid fire
 */

// Level 2 world scroll speed (slightly faster than Level 1)
const float L2_SCROLL_SPEED = 18.0f;

// Play area boundaries
const float Level2SolarDebrisPath::L2_PLAY_AREA_MIN_X = -55.0f;
const float Level2SolarDebrisPath::L2_PLAY_AREA_MAX_X = 55.0f;
const float Level2SolarDebrisPath::L2_PLAY_AREA_MIN_Y = -35.0f;
const float Level2SolarDebrisPath::L2_PLAY_AREA_MAX_Y = 35.0f;
const float Level2SolarDebrisPath::L2_SPAWN_DISTANCE_AHEAD = 180.0f;

Level2SolarDebrisPath::Level2SolarDebrisPath()
    : skybox(nullptr),
      satelliteTexture(0), solarPanelTexture(0), cometTexture(0),
      bulletTexture(0), collectibleTexture(0), portalTexture(0),
      collectedCellCount(0), requiredCellCount(L2_REQUIRED_CELLS),
      spawnedCells(0), spawnedPowerups(0),
      distanceTraveled(0.0f), nextCollectibleSpawnDistance(50.0f),
      portal(nullptr), portalBeingCaptured(false),
      rapidFireActive(false), rapidFireEndTime(0.0f),
      invincibilityActive(false), invincibilityEndTime(0.0f),
      powerupDuration(8.0f),  // 8 seconds for L2 power-ups
      shieldBubbleVAO(0), shieldBubbleVBO(0), shieldBubbleEBO(0), shieldBubbleIndexCount(0),
      scrollSpeed(L2_SCROLL_SPEED), worldZOffset(0.0f),
      completed(false), gameOver(false), levelTimer(0.0f),
      lastFireTime(0.0f), fireRate(0.15f), burstFireRate(0.05f), score(0),
      hudVAO(0), hudVBO(0), hudShaderProgram(0), spaceKeyWasPressed(false) {
}

Level2SolarDebrisPath::~Level2SolarDebrisPath() {
    cleanup();
}

void Level2SolarDebrisPath::init(Game& game) {
    std::cout << "Loading Level 2: Solar Debris Path..." << std::endl;
    
    // Create skybox with red space theme for Level 2
    // Order: right, left, top, bottom, front, back (all 6 faces)
    skybox = new Skybox();
    skybox->loadCubemap({
        "red/bkg3_right1.png",   // +X (right side)
        "red/bkg3_left2.png",    // -X (left side)
        "red/bkg3_top3.png",     // +Y (top)
        "red/bkg3_bottom4.png",  // -Y (bottom)
        "red/bkg3_front5.png",   // +Z (front - what you see ahead)
        "red/bkg3_back6.png"     // -Z (back - behind you)
    });
    
    // Load textures (using placeholder textures for now - can be replaced)
    // TODO: Load proper textures when available
    satelliteTexture = loadTexture("textures/metal.png");
    if (satelliteTexture == 0) {
        std::cout << "  Using fallback texture for satellites" << std::endl;
        satelliteTexture = loadTexture("Asteroid/Asteroid1e_Color_2K.png");
    }
    
    solarPanelTexture = satelliteTexture;  // Reuse for now
    cometTexture = loadTexture("textures/comet.png");
    if (cometTexture == 0) {
        cometTexture = loadTexture("Breakabel_Asteroids/Albedo.jpg");
    }
    
    bulletTexture = loadTexture("textures/bullet.png");
    if (bulletTexture == 0) {
        bulletTexture = 1;  // Fallback
    }
    
    collectibleTexture = loadTexture("textures/energy.png");
    if (collectibleTexture == 0) {
        collectibleTexture = loadTexture("textures/collectible.png");
    }
    
    portalTexture = loadTexture("textures/portal.png");
    if (portalTexture == 0) {
        portalTexture = 1;  // Fallback
    }
    
    // Load shared meshes (OBJ models)
    SatelliteFragment::loadSharedMesh();
    SwingSolarPanel::loadSharedMesh();
    Level2Collectible::loadSharedMeshes();
    
    // Initialize hazards
    initSatelliteFragments();
    initSolarPanels();
    initCometRocks();
    initCollectibles();
    initPortal();
    
    // Reset state
    completed = false;
    gameOver = false;
    blackHoleFreeze = false;
    levelTimer = 0.0f;
    lastFireTime = 0.0f;
    collectedCellCount = 0;
    portalBeingCaptured = false;
    distanceTraveled = 0.0f;
    nextCollectibleSpawnDistance = 50.0f;
    spawnedCells = 0;
    spawnedPowerups = 0;
    rapidFireActive = false;
    invincibilityActive = false;
    
    // NOTE: Don't reset player health here - preserve it from Level 1
    // Health is only reset in resetLevel() when player presses R
    
    // Clear projectiles
    bullets.clear();
    bullets.reserve(100);
    
    // Initialize HUD state
    hudState = Level2HUDState();
    hudState.totalCells = requiredCellCount;
    
    // Setup 2D HUD rendering
    setupHUD2D(game.getScreenWidth(), game.getScreenHeight());
    
    // Store screen dimensions for centered HUD elements
    hudScreenWidth = game.getScreenWidth();
    hudScreenHeight = game.getScreenHeight();
    
    // Initialize win screen text renderer
    winTextRenderer.init(game.getScreenWidth(), game.getScreenHeight());
    
    // Create shield bubble mesh for invincibility visual
    createShieldBubbleMesh();
    
    std::cout << "  Level 2 initialized with hazards and collectibles" << std::endl;
}

void Level2SolarDebrisPath::initSatelliteFragments() {
    satelliteFragments.clear();
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> yDist(L2_PLAY_AREA_MIN_Y * 0.8f, L2_PLAY_AREA_MAX_Y * 0.8f);
    std::uniform_real_distribution<float> zDist(-50.0f, -L2_SPAWN_DISTANCE_AHEAD);
    std::uniform_real_distribution<float> rotSpeedDist(20.0f, 60.0f);
    
    // Distribute satellites evenly across X axis including center
    float xSpacing = (L2_PLAY_AREA_MAX_X - L2_PLAY_AREA_MIN_X) / (L2_MAX_SATELLITE_FRAGMENTS + 1);
    for (int i = 0; i < L2_MAX_SATELLITE_FRAGMENTS; i++) {
        float x = L2_PLAY_AREA_MIN_X + xSpacing * (i + 1);
        // Add some random offset but keep general distribution
        x += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * xSpacing * 0.6f;
        glm::vec3 pos(x, yDist(gen), zDist(gen));
        float rotSpeed = rotSpeedDist(gen);
        satelliteFragments.emplace_back(pos, rotSpeed);
    }
    
    std::cout << "  Spawned " << L2_MAX_SATELLITE_FRAGMENTS << " satellite fragments" << std::endl;
}

void Level2SolarDebrisPath::initSolarPanels() {
    solarPanels.clear();
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> yDist(5.0f, 15.0f);  // High up so panels swing down
    std::uniform_real_distribution<float> zDist(-60.0f, -L2_SPAWN_DISTANCE_AHEAD);
    std::uniform_real_distribution<float> swingAngleDist(30.0f, 60.0f);
    std::uniform_real_distribution<float> swingSpeedDist(0.8f, 1.5f);
    
    // Distribute solar panels evenly across X axis including center
    float xSpacing = (L2_PLAY_AREA_MAX_X - L2_PLAY_AREA_MIN_X) / (L2_MAX_SOLAR_PANELS + 1);
    for (int i = 0; i < L2_MAX_SOLAR_PANELS; i++) {
        float x = L2_PLAY_AREA_MIN_X + xSpacing * (i + 1);
        // Add some random offset but keep general distribution
        x += (static_cast<float>(rand()) / RAND_MAX - 0.5f) * xSpacing * 0.5f;
        glm::vec3 pivotPos(x, yDist(gen), zDist(gen));
        float swingAngle = swingAngleDist(gen);
        float swingSpeed = swingSpeedDist(gen);
        solarPanels.emplace_back(pivotPos, swingAngle, swingSpeed);
    }
    
    std::cout << "  Spawned " << L2_MAX_SOLAR_PANELS << " swinging solar panels" << std::endl;
}

void Level2SolarDebrisPath::initCometRocks() {
    cometRocks.clear();
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(L2_PLAY_AREA_MIN_X, L2_PLAY_AREA_MAX_X);
    std::uniform_real_distribution<float> yDist(L2_PLAY_AREA_MIN_Y, L2_PLAY_AREA_MAX_Y);
    std::uniform_real_distribution<float> zDist(-80.0f, -L2_SPAWN_DISTANCE_AHEAD);
    std::uniform_real_distribution<float> speedDist(25.0f, 45.0f);  // Fast!
    std::uniform_real_distribution<float> radiusDist(0.5f, 1.2f);
    
    for (int i = 0; i < L2_MAX_COMET_ROCKS; i++) {
        glm::vec3 pos(xDist(gen), yDist(gen), zDist(gen));
        float speed = speedDist(gen);
        glm::vec3 velocity(0.0f, 0.0f, speed);  // Moving toward player (+Z)
        float radius = radiusDist(gen);
        cometRocks.emplace_back(pos, velocity, radius);
    }
    
    std::cout << "  Spawned " << L2_MAX_COMET_ROCKS << " comet rocks" << std::endl;
}

void Level2SolarDebrisPath::initCollectibles() {
    collectibles.clear();
    spawnedCells = 0;
    spawnedPowerups = 0;
    // Collectibles will spawn dynamically based on distance
}

void Level2SolarDebrisPath::initPortal() {
    // Black Hole portal spawns far ahead, locked until all cells collected
    glm::vec3 portalPos(0.0f, 0.0f, -500.0f);  // Very far ahead
    portal = new BlackHolePortal(portalPos);
    portal->setupMesh();
    // Portal starts hidden until all cells are collected
}

void Level2SolarDebrisPath::update(float dt, Game& game) {
    // Get player ship reference for use throughout update
    PlayerShip* ship = game.getPlayerShip();
    
    // Allow restart on win (completed) with R key, or return to intro screen with SPACE
    if (completed) {
        if (glfwGetKey(game.getWindow(), GLFW_KEY_R) == GLFW_PRESS) {
            resetLevel(game);
        }
        
        // SPACE key returns to intro screen (PLAY GAME button) after winning Level 2
        // Debouncing: only trigger on key press, not hold
        bool spaceKeyPressed = glfwGetKey(game.getWindow(), GLFW_KEY_SPACE) == GLFW_PRESS;
        if (spaceKeyPressed && !spaceKeyWasPressed) {
            game.returnToIntroScreen();
        }
        spaceKeyWasPressed = spaceKeyPressed;
        return;
    }
    
    levelTimer += dt;
    lastFireTime += dt;  // Track time since last bullet fired
    
    // Check for Game Over state (ship destroyed) - play sound once
    if (ship && !ship->getIsAlive() && !gameOver) {
        gameOver = true;
        hudState.shipAlive = false;
        game.playGameOverSound();  // Play game over sound
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════╗\n";
        std::cout << "║          GAME OVER                     ║\n";
        std::cout << "╚════════════════════════════════════════╝\n";
        std::cout << "\n";
    }
    
    // Handle game over restart
    if (gameOver) {
        if (glfwGetKey(game.getWindow(), GLFW_KEY_R) == GLFW_PRESS) {
            resetLevel(game);
        }
        return;
    }
    
    // ═══════════════════════════════════════════════════════════════
    //  ON-RAILS SCROLLING
    // ═══════════════════════════════════════════════════════════════
    
    // Check for slow-down (Left Shift reduces scroll speed temporarily)
    float currentScrollSpeed = scrollSpeed;
    if (glfwGetKey(game.getWindow(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        currentScrollSpeed = scrollSpeed * 0.4f;  // 40% speed when holding Shift
    }
    
    // Apply forward speed boost when jet booster burst is active
    if (ship && ship->isJetBoosterBurstActive()) {
        currentScrollSpeed *= 2.0f;  // Double forward speed during boost
    }
    
    float scrollDelta = currentScrollSpeed * dt;
    worldZOffset += scrollDelta;
    distanceTraveled += scrollDelta;
    
    // Scroll all objects toward player
    for (auto& fragment : satelliteFragments) {
        fragment.position.z += scrollDelta;
    }
    for (auto& panel : solarPanels) {
        panel.pivotPosition.z += scrollDelta;
    }
    for (auto& comet : cometRocks) {
        comet.position.z += scrollDelta;
    }
    for (auto& collectible : collectibles) {
        collectible.position.z += scrollDelta;
    }
    // Only scroll portal if it's active (not hidden)
    if (portal && portal->getState() != BlackHoleState::Hidden) {
        portal->position.z += scrollDelta;
    }
    
    // ═══════════════════════════════════════════════════════════════
    //  UPDATE ALL OBJECTS
    // ═══════════════════════════════════════════════════════════════
    
    // Update satellite fragments
    for (auto& fragment : satelliteFragments) {
        fragment.update(dt);
    }
    
    // Update solar panels
    for (auto& panel : solarPanels) {
        panel.update(dt);
    }
    
    // Update comet rocks
    for (auto& comet : cometRocks) {
        comet.update(dt);
    }
    
    // Update collectibles
    for (auto& collectible : collectibles) {
        collectible.update(dt);
    }
    
    // Update bullets
    for (auto& bullet : bullets) {
        bullet.update(dt);
    }
    
    // Update portal
    if (portal) {
        portal->update(dt);
        
        // Activate portal when all cells collected
        if (collectedCellCount >= requiredCellCount && portal->getState() == BlackHoleState::Hidden) {
            // Move portal to spawn far ahead of player
            portal->position = glm::vec3(0.0f, 0.0f, -200.0f);  // Spawn far ahead
            portal->spawn();
            hudState.portalActivatedFlashTimer = 1.0f;
            std::cout << "[BLACK HOLE] All cells collected! Portal opening at z=" << portal->position.z << std::endl;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    //  POWER-UP MANAGEMENT
    // ═══════════════════════════════════════════════════════════════
    
    if (rapidFireActive && levelTimer >= rapidFireEndTime) {
        rapidFireActive = false;
        hudState.rapidFireActive = false;
        std::cout << "[POWER-UP] Rapid Fire expired" << std::endl;
    }
    
    if (invincibilityActive && levelTimer >= invincibilityEndTime) {
        invincibilityActive = false;
        hudState.invincibilityActive = false;
        std::cout << "[POWER-UP] Invincibility Shield expired" << std::endl;
    }
    
    // ═══════════════════════════════════════════════════════════════
    //  PLAYER INPUT
    // ═══════════════════════════════════════════════════════════════
    
    // Note: Shooting is handled by Game.cpp (left mouse button)
    // Rapid Fire power-up is checked via isRapidFireActive() method
    
    // ═══════════════════════════════════════════════════════════════
    //  SPAWNING
    // ═══════════════════════════════════════════════════════════════
    
    float cameraZ = worldZOffset;
    updateSatelliteSpawning(cameraZ);
    updateSolarPanelSpawning(cameraZ);
    updateCometSpawning(cameraZ);
    updateCollectibleSpawning();
    
    // ═══════════════════════════════════════════════════════════════
    //  COLLISION DETECTION
    // ═══════════════════════════════════════════════════════════════
    
    checkCollisions(game);
    
    // ═══════════════════════════════════════════════════════════════
    //  CLEANUP DEAD OBJECTS
    // ═══════════════════════════════════════════════════════════════
    
    // Remove expired bullets
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) { return b.isExpired(); }),
        bullets.end()
    );
    
    // Remove dead comets
    cometRocks.erase(
        std::remove_if(cometRocks.begin(), cometRocks.end(),
            [](const CometRock& c) { return c.isDead(); }),
        cometRocks.end()
    );
    
    // Remove collected collectibles
    collectibles.erase(
        std::remove_if(collectibles.begin(), collectibles.end(),
            [](const Level2Collectible& c) { return c.isReadyToRemove(); }),
        collectibles.end()
    );
    
    // ═══════════════════════════════════════════════════════════════
    //  CHECK WIN CONDITION
    // ═══════════════════════════════════════════════════════════════
    
    if (portal && portal->getState() == BlackHoleState::Active) {
        PlayerShip* ship = game.getPlayerShip();
        if (portal->checkPlayerEntry(ship->getPosition(), 1.5f)) {
            portal->startCapture();
            portalBeingCaptured = true;
        }
    }
    
    // Pull player into black hole center during capture
    // Also suck in all nearby objects for visual effect
    if (portal && portal->getState() == BlackHoleState::Capturing) {
        PlayerShip* ship = game.getPlayerShip();
        glm::vec3 newPos = portal->pullPlayerToward(ship->getPosition(), dt);
        ship->setPosition(newPos);
        
        // Black hole position
        glm::vec3 blackHolePos = portal->getPosition();
        float suckSpeed = 200.0f;  // Speed at which objects get pulled in (faster!)
        float destroyRadius = 15.0f;  // Destroy objects when they get close enough
        float suckRadius = 500.0f;  // Everything within this radius gets pulled in
        
        // Suck in satellite fragments
        for (auto& fragment : satelliteFragments) {
            if (fragment.isDestroyed()) continue;
            glm::vec3 toHole = blackHolePos - fragment.position;
            float dist = glm::length(toHole);
            if (dist < destroyRadius) {
                fragment.destroy();  // Object consumed by black hole
            } else if (dist < suckRadius) {
                // Pull toward black hole - faster when closer
                glm::vec3 dir = glm::normalize(toHole);
                float pullStrength = suckSpeed * (1.0f + 100.0f / dist);  // Stronger pull when closer
                fragment.position += dir * pullStrength * dt;
            }
        }
        
        // Suck in solar panels
        for (auto& panel : solarPanels) {
            if (panel.isDestroyed()) continue;
            glm::vec3 toHole = blackHolePos - panel.pivotPosition;
            float dist = glm::length(toHole);
            if (dist < destroyRadius) {
                panel.destroy();  // Object consumed by black hole
            } else if (dist < suckRadius) {
                // Pull toward black hole
                glm::vec3 dir = glm::normalize(toHole);
                float pullStrength = suckSpeed * (1.0f + 100.0f / dist);
                panel.pivotPosition += dir * pullStrength * dt;
            }
        }
        
        // Suck in comet rocks
        for (auto& comet : cometRocks) {
            if (comet.isDead()) continue;
            glm::vec3 toHole = blackHolePos - comet.getPosition();
            float dist = glm::length(toHole);
            if (dist < destroyRadius) {
                comet.destroy();  // Object consumed by black hole
            } else if (dist < suckRadius) {
                // Pull toward black hole
                float pullStrength = suckSpeed * (1.0f + 100.0f / dist);
                comet.moveToward(blackHolePos, pullStrength * dt);
            }
        }
        
        // Suck in collectibles
        for (auto& collectible : collectibles) {
            if (collectible.isReadyToRemove()) continue;
            glm::vec3 toHole = blackHolePos - collectible.getPosition();
            float dist = glm::length(toHole);
            if (dist < destroyRadius) {
                collectible.collect();  // Object consumed by black hole
            } else if (dist < suckRadius) {
                // Pull toward black hole - move position directly
                glm::vec3 dir = glm::normalize(toHole);
                float pullStrength = suckSpeed * (1.0f + 100.0f / dist);
                collectible.forceMove(dir * pullStrength * dt);
            }
        }
        
        // Suck in bullets too
        for (auto& bullet : bullets) {
            glm::vec3 toHole = blackHolePos - bullet.getPosition();
            float dist = glm::length(toHole);
            if (dist < suckRadius) {
                float pullStrength = suckSpeed * (1.0f + 100.0f / dist);
                bullet.pullToward(blackHolePos, pullStrength * dt);
            }
        }
    }
    
    if (portal && portal->getState() == BlackHoleState::Completed) {
        // Level complete - show You Win screen
        completed = true;
        std::cout << "🎉 Level 2 Complete! Congratulations - YOU WIN! 🎉" << std::endl;
    }
    
    // Update HUD
    updateHUD(dt, game);
}

void Level2SolarDebrisPath::updateSatelliteSpawning(float cameraZ) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    // Use full range including center
    std::uniform_real_distribution<float> xDist(L2_PLAY_AREA_MIN_X * 0.9f, L2_PLAY_AREA_MAX_X * 0.9f);
    std::uniform_real_distribution<float> yDist(L2_PLAY_AREA_MIN_Y * 0.8f, L2_PLAY_AREA_MAX_Y * 0.8f);
    std::uniform_real_distribution<float> rotSpeedDist(20.0f, 60.0f);
    
    for (auto& fragment : satelliteFragments) {
        // Respawn if passed behind camera
        if (fragment.position.z > 20.0f) {
            float newZ = -L2_SPAWN_DISTANCE_AHEAD;
            fragment.respawn(glm::vec3(xDist(gen), yDist(gen), newZ), rotSpeedDist(gen));
        }
    }
}

void Level2SolarDebrisPath::updateSolarPanelSpawning(float cameraZ) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    // Use full range including center area
    std::uniform_real_distribution<float> xDist(L2_PLAY_AREA_MIN_X * 0.9f, L2_PLAY_AREA_MAX_X * 0.9f);
    std::uniform_real_distribution<float> yDist(5.0f, 15.0f);
    std::uniform_real_distribution<float> swingAngleDist(30.0f, 60.0f);
    std::uniform_real_distribution<float> swingSpeedDist(0.8f, 1.5f);
    
    for (auto& panel : solarPanels) {
        if (panel.pivotPosition.z > 20.0f) {
            float newZ = -L2_SPAWN_DISTANCE_AHEAD;
            panel.respawn(glm::vec3(xDist(gen), yDist(gen), newZ),
                         swingAngleDist(gen), swingSpeedDist(gen));
        }
    }
}

void Level2SolarDebrisPath::updateCometSpawning(float cameraZ) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(L2_PLAY_AREA_MIN_X, L2_PLAY_AREA_MAX_X);
    std::uniform_real_distribution<float> yDist(L2_PLAY_AREA_MIN_Y, L2_PLAY_AREA_MAX_Y);
    std::uniform_real_distribution<float> speedDist(25.0f, 45.0f);
    std::uniform_real_distribution<float> radiusDist(0.5f, 1.2f);
    
    for (auto& comet : cometRocks) {
        // Respawn if passed behind camera or dead
        if (comet.position.z > 30.0f) {
            float newZ = -L2_SPAWN_DISTANCE_AHEAD;
            float speed = speedDist(gen);
            comet.respawn(
                glm::vec3(xDist(gen), yDist(gen), newZ),
                glm::vec3(0.0f, 0.0f, speed),
                radiusDist(gen)
            );
        }
    }
}

void Level2SolarDebrisPath::updateCollectibleSpawning() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(L2_PLAY_AREA_MIN_X * 0.6f, L2_PLAY_AREA_MAX_X * 0.6f);
    std::uniform_real_distribution<float> yDist(L2_PLAY_AREA_MIN_Y * 0.5f, L2_PLAY_AREA_MAX_Y * 0.5f);
    std::uniform_real_distribution<float> typeDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> nextSpawnDist(40.0f, 80.0f);
    
    // Respawn missed collectibles (passed behind camera without being collected)
    for (auto& collectible : collectibles) {
        if (!collectible.isCollected() && collectible.getPosition().z > 30.0f) {
            // Respawn ahead - this collectible was missed
            glm::vec3 newPos(xDist(gen), yDist(gen), -L2_SPAWN_DISTANCE_AHEAD * 0.8f);
            collectible.respawn(newPos);
        }
    }
    
    // Also spawn new collectibles periodically until we have enough
    if (distanceTraveled < nextCollectibleSpawnDistance) return;
    
    // Only stop spawning cells if player has collected enough
    bool needMoreCells = (collectedCellCount < requiredCellCount);
    bool spawnPowerup = (spawnedPowerups < L2_MAX_POWERUPS_TO_SPAWN);
    
    if (!needMoreCells && !spawnPowerup) {
        nextCollectibleSpawnDistance = distanceTraveled + nextSpawnDist(gen);
        return;
    }
    
    // Decide type - cells are more common
    Level2CollectibleType type;
    if (needMoreCells && (!spawnPowerup || typeDist(gen) < 0.75f)) {
        type = Level2CollectibleType::RedEnergyCell;
        spawnedCells++;
    } else if (spawnPowerup) {
        // 50/50 chance for RapidFire or Invincibility
        std::uniform_int_distribution<int> powerupTypeDist(0, 1);
        type = (powerupTypeDist(gen) == 0) 
            ? Level2CollectibleType::RapidFire 
            : Level2CollectibleType::Invincibility;
        spawnedPowerups++;
        std::cout << "[SPAWN] Power-up: " 
                  << (type == Level2CollectibleType::RapidFire ? "Rapid Fire" : "Invincibility")
                  << " spawned!" << std::endl;
    } else {
        return;
    }
    
    glm::vec3 pos(xDist(gen), yDist(gen), -L2_SPAWN_DISTANCE_AHEAD * 0.8f);
    collectibles.emplace_back(pos, type);
    collectibles.back().setupMesh();
    
    nextCollectibleSpawnDistance = distanceTraveled + nextSpawnDist(gen);
}

void Level2SolarDebrisPath::fireBullet(const glm::vec3& position, const glm::vec3& direction) {
    // Don't allow firing when Game Over
    if (gameOver) {
        return;
    }
    
    // Fire rate is now handled by Game.cpp - no duplicate checking here
    // This function just spawns the bullet
    
    float bulletSpeed = 60.0f;  // Fast bullets
    
    // Normal single bullet (rapid fire just removes cooldown, doesn't change shot type)
    glm::vec3 spawnPos = position + direction * 1.2f;
    bullets.emplace_back(spawnPos, direction, bulletSpeed);
}

void Level2SolarDebrisPath::checkCollisions(Game& game) {
    checkBulletCollisions();
    checkCollectibleCollisions(game);
    checkHazardCollisions(game);
}

void Level2SolarDebrisPath::checkBulletCollisions() {
    for (auto& bullet : bullets) {
        if (bullet.isExpired()) continue;
        
        glm::vec3 bulletPos = bullet.getPosition();
        float bulletRadius = bullet.getRadius();
        
        // Check against comet rocks
        for (auto& comet : cometRocks) {
            if (comet.getState() != CometRock::State::Normal) continue;
            
            if (comet.checkCollision(bulletPos, bulletRadius)) {
                comet.startBreaking();
                bullet.markForDestruction();
                score += 10;  // 10 points per comet destroyed
                std::cout << "[BULLET] Hit comet! Score: " << score << std::endl;
                break;
            }
        }
        
        if (bullet.isExpired()) continue;  // Already destroyed
        
        // Check against satellite fragments
        for (auto& fragment : satelliteFragments) {
            glm::vec3 fragPos = fragment.getPosition();
            float dist = glm::length(bulletPos - fragPos);
            float combinedRadius = fragment.getCollisionRadius() + bulletRadius;
            
            if (dist < combinedRadius) {
                bullet.markForDestruction();
                std::cout << "[BULLET] Hit satellite!" << std::endl;
                break;
            }
        }
        
        if (bullet.isExpired()) continue;  // Already destroyed
        
        // Check against solar panels
        for (auto& panel : solarPanels) {
            glm::vec3 panelPos = panel.getPanelCenterPosition();
            float dist = glm::length(bulletPos - panelPos);
            float combinedRadius = panel.getCollisionRadius() + bulletRadius;
            
            if (dist < combinedRadius) {
                bullet.markForDestruction();
                std::cout << "[BULLET] Hit solar panel!" << std::endl;
                break;
            }
        }
    }
}

void Level2SolarDebrisPath::checkCollectibleCollisions(Game& game) {
    PlayerShip* ship = game.getPlayerShip();
    glm::vec3 playerPos = ship->getPosition();
    float playerRadius = 1.5f;
    
    for (auto& collectible : collectibles) {
        if (collectible.isCollected()) continue;
        
        if (collectible.checkCollision(playerPos, playerRadius)) {
            collectible.collect();
            
            switch (collectible.getType()) {
                case Level2CollectibleType::RedEnergyCell:
                    game.playCollectSound();  // Play collect sound
                    collectedCellCount++;
                    score += 200;  // 200 points per collectible in Level 2
                    hudState.collectedCells = collectedCellCount;
                    hudState.cellCollectedFlashTimer = 0.3f;
                    std::cout << "[COLLECT] Energy Cells: " << collectedCellCount 
                              << "/" << requiredCellCount << " | Score: " << score << std::endl;
                    break;
                    
                case Level2CollectibleType::RapidFire:
                    game.playPowerupSound();  // Play powerup sound
                    rapidFireActive = true;
                    rapidFireEndTime = levelTimer + powerupDuration;
                    score += 200;  // 200 points per collectible in Level 2
                    hudState.rapidFireActive = true;
                    hudState.rapidFireTimeRemaining = powerupDuration;
                    hudState.powerupStartFlashTimer = 0.5f;
                    std::cout << "[POWERUP] Rapid Fire active for " << powerupDuration << " seconds! | Score: " << score << std::endl;
                    break;
                    
                case Level2CollectibleType::Invincibility:
                    game.playPowerupSound();  // Play powerup sound
                    invincibilityActive = true;
                    invincibilityEndTime = levelTimer + powerupDuration;
                    score += 200;  // 200 points per collectible in Level 2
                    hudState.invincibilityActive = true;
                    hudState.invincibilityTimeRemaining = powerupDuration;
                    hudState.powerupStartFlashTimer = 0.5f;
                    std::cout << "[POWERUP] Invincibility Shield active for " << powerupDuration << " seconds! | Score: " << score << std::endl;
                    break;
            }
        }
    }
}

void Level2SolarDebrisPath::checkHazardCollisions(Game& game) {
    PlayerShip* ship = game.getPlayerShip();
    if (!ship->getIsAlive() || ship->getIsInvulnerable() || gameOver) return;
    
    // Skip collision damage if invincibility power-up is active
    if (invincibilityActive) return;
    
    glm::vec3 playerPos = ship->getPosition();
    float playerRadius = 1.5f;  // Tight player hitbox matching ship size
    
    // Check satellite fragments
    for (auto& fragment : satelliteFragments) {
        if (fragment.isDestroyed()) continue;  // Skip destroyed fragments
        
        glm::vec3 fragPos = fragment.getPosition();
        float dist = glm::length(playerPos - fragPos);
        float combinedRadius = fragment.getCollisionRadius() + playerRadius;
        
        if (dist < combinedRadius) {
            game.playCrashSound();  // Play crash sound
            ship->takeDamage(1);
            fragment.destroy();  // Destroy the satellite fragment
            
            // Bounce player away from satellite
            glm::vec3 bounceDir = glm::normalize(playerPos - fragPos);
            glm::vec3 newPos = playerPos + bounceDir * 3.0f;  // Push player back
            ship->setPosition(newPos);
            
            std::cout << "[COLLISION] Hit satellite fragment - destroyed! HP: " 
                      << ship->getHealth() << std::endl;
            
            if (!ship->getIsAlive()) {
                gameOver = true;
                hudState.shipAlive = false;
                game.playGameOverSound();  // Play game over sound
                std::cout << "GAME OVER" << std::endl;
            }
            return;
        }
    }
    
    // Check solar panels
    for (auto& panel : solarPanels) {
        if (panel.isDestroyed()) continue;  // Skip destroyed panels
        
        glm::vec3 panelPos = panel.getPanelCenterPosition();
        float dist = glm::length(playerPos - panelPos);
        float combinedRadius = panel.getCollisionRadius() + playerRadius;
        
        if (dist < combinedRadius) {
            game.playCrashSound();  // Play crash sound
            ship->takeDamage(1);
            panel.destroy();  // Destroy the solar panel
            
            // Bounce player away from panel
            glm::vec3 bounceDir = glm::normalize(playerPos - panelPos);
            glm::vec3 newPos = playerPos + bounceDir * 3.0f;  // Push player back
            ship->setPosition(newPos);
            
            std::cout << "[COLLISION] Hit solar panel - destroyed! HP: " 
                      << ship->getHealth() << std::endl;
            
            if (!ship->getIsAlive()) {
                gameOver = true;
                hudState.shipAlive = false;
                game.playGameOverSound();  // Play game over sound
                std::cout << "GAME OVER" << std::endl;
            }
            return;
        }
    }
    
    // Check comet rocks - use swept collision for fast-moving comets
    for (auto& comet : cometRocks) {
        if (comet.getState() != CometRock::State::Normal) continue;
        
        // Use swept collision detection (checks path traveled)
        if (comet.checkPlayerCollision(playerPos, playerRadius)) {
            game.playCrashSound();  // Play crash sound
            ship->takeDamage(1);
            comet.startBreaking();  // Comet breaks on player collision
            
            // Bounce player away from comet
            glm::vec3 cometPos = comet.getPosition();
            glm::vec3 bounceDir = glm::normalize(playerPos - cometPos);
            glm::vec3 newPos = playerPos + bounceDir * 3.0f;  // Push player back
            ship->setPosition(newPos);
            
            std::cout << "[COLLISION] Hit comet - destroyed! HP: " 
                      << ship->getHealth() << std::endl;
            
            if (!ship->getIsAlive()) {
                gameOver = true;
                hudState.shipAlive = false;
                game.playGameOverSound();  // Play game over sound
                std::cout << "GAME OVER" << std::endl;
            }
            return;
        }
    }
}

void Level2SolarDebrisPath::updateHUD(float dt, Game& game) {
    PlayerShip* ship = game.getPlayerShip();
    
    hudState.shipHealth = ship->getHealth();
    hudState.shipInvulnerable = ship->getIsInvulnerable();
    hudState.shipAlive = ship->getIsAlive();
    hudState.collectedCells = collectedCellCount;
    hudState.portalActive = (portal && portal->isActive());
    
    // Update power-up states
    if (rapidFireActive) {
        hudState.rapidFireActive = true;
        hudState.rapidFireTimeRemaining = rapidFireEndTime - levelTimer;
    } else {
        hudState.rapidFireActive = false;
        hudState.rapidFireTimeRemaining = 0.0f;
    }
    
    if (invincibilityActive) {
        hudState.invincibilityActive = true;
        hudState.invincibilityTimeRemaining = invincibilityEndTime - levelTimer;
    } else {
        hudState.invincibilityActive = false;
        hudState.invincibilityTimeRemaining = 0.0f;
    }
    
    // Decay flash timers
    if (hudState.cellCollectedFlashTimer > 0.0f) {
        hudState.cellCollectedFlashTimer -= dt;
    }
    if (hudState.powerupStartFlashTimer > 0.0f) {
        hudState.powerupStartFlashTimer -= dt;
    }
    if (hudState.portalActivatedFlashTimer > 0.0f) {
        hudState.portalActivatedFlashTimer -= dt;
    }
}

void Level2SolarDebrisPath::resetLevel(Game& game) {
    std::cout << "Restarting Level 2..." << std::endl;
    
    // Reset state
    completed = false;
    gameOver = false;
    blackHoleFreeze = false;
    levelTimer = 0.0f;
    lastFireTime = 0.0f;
    collectedCellCount = 0;
    spawnedCells = 0;
    spawnedPowerups = 0;
    distanceTraveled = 0.0f;
    nextCollectibleSpawnDistance = 50.0f;
    worldZOffset = 0.0f;
    portalBeingCaptured = false;
    rapidFireActive = false;
    invincibilityActive = false;
    score = 0;  // Reset score
    
    // Reset player
    game.getPlayerShip()->resetHealth();
    
    // Clear and respawn objects
    bullets.clear();
    collectibles.clear();
    
    // Reinitialize hazards
    satelliteFragments.clear();
    solarPanels.clear();
    cometRocks.clear();
    
    initSatelliteFragments();
    initSolarPanels();
    initCometRocks();
    
    // Reset portal
    if (portal) {
        delete portal;
    }
    initPortal();
    
    // Reset HUD
    hudState = Level2HUDState();
    hudState.totalCells = requiredCellCount;
}

void Level2SolarDebrisPath::render(Game& game) {
    // Get view and projection matrices
    glm::mat4 view = game.getCameraController()->getViewMatrix();
    float aspect = (float)game.getScreenWidth() / (float)game.getScreenHeight();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
    
    // Render skybox
    if (skybox) {
        skybox->render(view, projection);
    }
    
    // Get shader from Game (same pattern as Level1)
    Shader* shader = game.getTexturedShader();
    if (!shader) {
        std::cerr << "[ERROR] Shader is null in Level2 render!" << std::endl;
        return;
    }
    
    // Get PBR shader for satellite and solar panel rendering
    Shader* pbrShader = game.getPBRShader();
    
    // Render satellite fragments with PBR shader
    if (pbrShader && SatelliteFragment::isSharedMeshLoaded()) {
        pbrShader->use();
        pbrShader->setMat4("view", view);
        pbrShader->setMat4("projection", projection);
        
        // Set lighting uniforms
        glm::vec3 lightPos = glm::vec3(50.0f, 100.0f, 50.0f);
        glm::vec3 lightColor = glm::vec3(1.0f, 0.98f, 0.95f);
        glm::vec3 viewPos = game.getCameraController()->position;
        
        pbrShader->setVec3("lightPos", lightPos);
        pbrShader->setVec3("lightColor", lightColor);
        pbrShader->setVec3("viewPos", viewPos);
        
        // Set texture samplers
        pbrShader->setInt("albedoMap", 0);
        pbrShader->setInt("metallicMap", 1);
        pbrShader->setInt("roughnessMap", 2);
        
        // Default material values (fallback)
        pbrShader->setVec3("albedoColor", glm::vec3(0.8f));
        pbrShader->setFloat("metallicValue", 0.5f);
        pbrShader->setFloat("roughnessValue", 0.5f);
        pbrShader->setInt("hasNormalMap", 0);
        
        // Get satellite materials for texture flags
        const auto& satMaterials = SatelliteFragment::getMaterials();
        const auto& satSubmeshes = SatelliteFragment::getSubMeshes();
        
        for (auto& fragment : satelliteFragments) {
            if (fragment.isDestroyed()) continue;  // Skip destroyed fragments
            
            pbrShader->setMat4("model", fragment.getModelMatrix());
            
            // Render each submesh with proper texture flags
            for (const auto& submesh : satSubmeshes) {
                if (submesh.materialIndex >= 0 && submesh.materialIndex < (int)satMaterials.size()) {
                    const auto& mat = satMaterials[submesh.materialIndex];
                    
                    // Set texture availability flags
                    pbrShader->setInt("hasAlbedo", mat.diffuseTexture != 0 ? 1 : 0);
                    pbrShader->setInt("hasMetallic", mat.metallicTexture != 0 ? 1 : 0);
                    pbrShader->setInt("hasRoughness", mat.roughnessTexture != 0 ? 1 : 0);
                    
                    // Bind textures
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture != 0 ? mat.diffuseTexture : 0);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, mat.metallicTexture != 0 ? mat.metallicTexture : 0);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, mat.roughnessTexture != 0 ? mat.roughnessTexture : 0);
                } else {
                    pbrShader->setInt("hasAlbedo", 0);
                    pbrShader->setInt("hasMetallic", 0);
                    pbrShader->setInt("hasRoughness", 0);
                }
                
                glBindVertexArray(submesh.VAO);
                glDrawElements(GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }
        glActiveTexture(GL_TEXTURE0);
    } else {
        // Fallback to regular shader
        shader->use();
        for (auto& fragment : satelliteFragments) {
            if (fragment.isDestroyed()) continue;  // Skip destroyed fragments
            shader->setMat4("model", fragment.getModelMatrix());
            fragment.render(satelliteTexture);
        }
    }
    
    // Render solar panels with regular textured shader (simpler, no flickering)
    shader->use();
    shader->setInt("isSolarPanel", 1);  // Enable darkening for solar panels
    for (auto& panel : solarPanels) {
        if (panel.isDestroyed()) continue;  // Skip destroyed panels
        shader->setMat4("model", panel.getModelMatrix());
        panel.render(solarPanelTexture);
    }
    shader->setInt("isSolarPanel", 0);  // Reset for other objects
    
    // Render comet rocks
    for (auto& comet : cometRocks) {
        if (comet.isDead()) continue;
        shader->setMat4("model", comet.getModelMatrix());
        comet.render(cometTexture);
    }
    
    // Render collectibles with glow effect
    float glowPulse = (sin(levelTimer * 4.0f) + 1.0f) / 2.0f;  // Pulse 0-1
    shader->setInt("isCollectible", 1);
    shader->setFloat("glowPulse", glowPulse);
    
    for (auto& collectible : collectibles) {
        if (collectible.isCollected()) continue;
        shader->setMat4("model", collectible.getModelMatrix());
        
        // Red glow for energy cells, green for power-ups
        if (collectible.getType() == Level2CollectibleType::RedEnergyCell) {
            shader->setVec3("collectibleGlow", glm::vec3(1.0f, 0.3f, 0.3f));  // Red
        } else {
            shader->setVec3("collectibleGlow", glm::vec3(0.3f, 1.0f, 0.3f));  // Green
        }
        
        collectible.render(collectibleTexture);
    }
    shader->setInt("isCollectible", 0);  // Reset
    
    // Render bullets
    for (size_t i = 0; i < bullets.size(); ++i) {
        if (bullets[i].isExpired()) continue;
        shader->setMat4("model", bullets[i].getModelMatrix());
        bullets[i].render(bulletTexture);
    }
    
    // Render portal
    if (portal && portal->getState() != BlackHoleState::Hidden) {
        shader->setMat4("model", portal->getModelMatrix());
        portal->render(shader->ID);
    }
    
    // Render invincibility shield bubble around player
    if (invincibilityActive && shieldBubbleVAO != 0) {
        PlayerShip* ship = game.getPlayerShip();
        glm::vec3 playerPos = ship->getPosition();
        
        // Enable transparency for shield
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);  // Don't write to depth buffer
        
        // Create model matrix centered on player
        glm::mat4 shieldModel = glm::translate(glm::mat4(1.0f), playerPos);
        shader->setMat4("model", shieldModel);
        
        // Set shield bubble uniforms
        shader->setInt("isShieldBubble", 1);
        float shieldPulseValue = (sin(levelTimer * 6.0f) + 1.0f) / 2.0f;
        shader->setFloat("shieldPulse", shieldPulseValue);
        
        // Draw shield bubble
        glBindVertexArray(shieldBubbleVAO);
        glDrawElements(GL_TRIANGLES, shieldBubbleIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        // Reset state
        shader->setInt("isShieldBubble", 0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    
    // Render rapid fire effect (orange aura around ship)
    if (rapidFireActive && shieldBubbleVAO != 0) {
        PlayerShip* ship = game.getPlayerShip();
        glm::vec3 playerPos = ship->getPosition();
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        
        // Smaller aura for rapid fire (scale 0.6)
        glm::mat4 rapidModel = glm::translate(glm::mat4(1.0f), playerPos);
        rapidModel = glm::scale(rapidModel, glm::vec3(0.6f));
        shader->setMat4("model", rapidModel);
        
        shader->setInt("isRapidFireEffect", 1);
        float rapidPulseValue = (sin(levelTimer * 12.0f) + 1.0f) / 2.0f;  // Faster pulse
        shader->setFloat("rapidFirePulse", rapidPulseValue);
        
        glBindVertexArray(shieldBubbleVAO);
        glDrawElements(GL_TRIANGLES, shieldBubbleIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        shader->setInt("isRapidFireEffect", 0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
    
    // Render 2D HUD overlay
    renderHUD(game);
}

bool Level2SolarDebrisPath::isCompleted() const {
    return completed;
}

std::string Level2SolarDebrisPath::getLevelName() const {
    return "Wave 2 - Solar Debris Path";
}

LevelType Level2SolarDebrisPath::getLevelType() const {
    return LevelType::Level2_SolarDebrisPath;
}

void Level2SolarDebrisPath::createShieldBubbleMesh() {
    // Create a simple UV sphere for the shield bubble
    const int segments = 16;
    const int rings = 12;
    const float radius = 3.5f;  // Shield bubble radius around player
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Generate vertices
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = glm::pi<float>() * ring / rings;
        float y = cos(phi) * radius;
        float ringRadius = sin(phi) * radius;
        
        for (int seg = 0; seg <= segments; ++seg) {
            float theta = 2.0f * glm::pi<float>() * seg / segments;
            float x = cos(theta) * ringRadius;
            float z = sin(theta) * ringRadius;
            
            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            
            // Normal (pointing outward)
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
            vertices.push_back(normal.x);
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);
            
            // Texture coords (not used but required by shader)
            vertices.push_back((float)seg / segments);
            vertices.push_back((float)ring / rings);
        }
    }
    
    // Generate indices
    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    shieldBubbleIndexCount = indices.size();
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &shieldBubbleVAO);
    glGenBuffers(1, &shieldBubbleVBO);
    glGenBuffers(1, &shieldBubbleEBO);
    
    glBindVertexArray(shieldBubbleVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, shieldBubbleVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shieldBubbleEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
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
    
    std::cout << "[SHIELD] Shield bubble mesh created with " << shieldBubbleIndexCount << " indices" << std::endl;
}

void Level2SolarDebrisPath::cleanup() {
    // Cleanup HUD
    cleanupHUD2D();
    
    // Cleanup shield bubble mesh
    if (shieldBubbleVAO != 0) {
        glDeleteVertexArrays(1, &shieldBubbleVAO);
        shieldBubbleVAO = 0;
    }
    if (shieldBubbleVBO != 0) {
        glDeleteBuffers(1, &shieldBubbleVBO);
        shieldBubbleVBO = 0;
    }
    if (shieldBubbleEBO != 0) {
        glDeleteBuffers(1, &shieldBubbleEBO);
        shieldBubbleEBO = 0;
    }
    
    if (skybox) {
        delete skybox;
        skybox = nullptr;
    }
    
    if (portal) {
        portal->cleanup();
        delete portal;
        portal = nullptr;
    }
    
    satelliteFragments.clear();
    solarPanels.clear();
    cometRocks.clear();
    collectibles.clear();
    bullets.clear();
    
    // Cleanup shared meshes
    SatelliteFragment::cleanupSharedMesh();
    SwingSolarPanel::cleanupSharedMesh();
    
    // Delete textures
    if (satelliteTexture != 0) {
        glDeleteTextures(1, &satelliteTexture);
        satelliteTexture = 0;
    }
    if (cometTexture != 0 && cometTexture != satelliteTexture) {
        glDeleteTextures(1, &cometTexture);
        cometTexture = 0;
    }
    // Note: Other textures may be shared, careful cleanup
}

glm::vec3 Level2SolarDebrisPath::getPlayerSpawnPosition() const {
    return glm::vec3(0.0f, 0.0f, -10.0f);
}

glm::vec3 Level2SolarDebrisPath::getPlayerSpawnRotation() const {
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

// ═══════════════════════════════════════════════════════════════════════
//  2D HUD RENDERING - Simple colored bars (same pattern as Level 1)
// ═══════════════════════════════════════════════════════════════════════

void Level2SolarDebrisPath::setupHUD2D(int screenWidth, int screenHeight) {
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

void Level2SolarDebrisPath::cleanupHUD2D() {
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

void Level2SolarDebrisPath::drawHUDQuad(float x, float y, float width, float height, float r, float g, float b) {
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

void Level2SolarDebrisPath::renderHUD(Game& game) {
    /*
     * ═══════════════════════════════════════════════════════════════
     *  SIMPLE 2D HUD - Colored bars in orthographic screen space
     * ═══════════════════════════════════════════════════════════════
     */
    
    // Keep window title for debug
    std::string title = "Starwave 3D - Level 2 | ";
    title += "HP: " + std::to_string(hudState.shipHealth) + "/" + std::to_string(hudState.shipMaxHealth);
    if (hudState.shipInvulnerable) title += " [INVULN]";
    if (!hudState.shipAlive) title += " [DEAD]";
    title += " | Cells: " + std::to_string(hudState.collectedCells) + "/" + std::to_string(hudState.totalCells);
    if (hudState.rapidFireActive) {
        title += " | RAPID: " + std::to_string((int)hudState.rapidFireTimeRemaining) + "s";
    }
    if (hudState.invincibilityActive) {
        title += " | SHIELD: " + std::to_string((int)hudState.invincibilityTimeRemaining) + "s";
    }
    title += " | Portal: ";
    if (!portal) title += "LOCKED";
    else if (portal->getState() == BlackHoleState::Activating) title += "SPAWNING";
    else title += hudState.portalActive ? "ACTIVE" : "LOCKED";
    glfwSetWindowTitle(game.getWindow(), title.c_str());
    
    // Render simple 2D HUD overlay
    renderHUD2D();
}

void Level2SolarDebrisPath::renderHUD2D() {
    if (hudShaderProgram == 0) return;
    
    // Save OpenGL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(hudShaderProgram);
    
    // Orthographic projection using stored screen dimensions
    glm::mat4 projection = glm::ortho(0.0f, (float)hudScreenWidth, (float)hudScreenHeight, 0.0f);
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
    
    // Energy Cell bars (6 segments - red theme for Level 2) - Enhanced
    float cellWidth = 30.0f;
    float cellSpacing = 35.0f;
    for (int i = 0; i < hudState.totalCells; ++i) {
        float xPos = margin + i * cellSpacing;
        
        // Outer border (darker)
        drawHUDQuad(xPos - 2.0f, yPos - 2.0f, cellWidth + 4.0f, barHeight + 4.0f, 0.1f, 0.1f, 0.1f);
        
        if (i < hudState.collectedCells) {
            // Collected: Red with glow
            float pulse = 0.9f + 0.1f * sin(levelTimer * 5.0f + i * 0.5f);
            // Glow effect (behind)
            drawHUDQuad(xPos - 3.0f, yPos - 3.0f, cellWidth + 6.0f, barHeight + 6.0f, 1.0f * 0.3f * pulse, 0.2f * 0.3f * pulse, 0.2f * 0.3f * pulse);
            // Main bar with gradient (brighter top)
            drawHUDQuad(xPos, yPos, cellWidth, barHeight * 0.5f, 1.0f * pulse, 0.3f, 0.3f);  // Top half
            drawHUDQuad(xPos, yPos + barHeight * 0.5f, cellWidth, barHeight * 0.5f, 0.8f * pulse, 0.1f, 0.1f);  // Bottom half
        } else {
            // Empty: Dark gray
            drawHUDQuad(xPos, yPos, cellWidth, barHeight, 0.25f, 0.25f, 0.25f);
            // Inner shadow
            drawHUDQuad(xPos, yPos, cellWidth, 3.0f, 0.15f, 0.15f, 0.15f);
        }
    }
    yPos += barHeight + 10.0f;
    
    // Rapid Fire power-up bar (when active) - Orange with name label
    if (hudState.rapidFireActive && hudState.rapidFireTimeRemaining > 0.0f) {
        float maxWidth = 150.0f;
        float fillRatio = hudState.rapidFireTimeRemaining / 2.0f;  // 2 second duration
        float fillWidth = maxWidth * fillRatio;
        
        // Power-up name label above the bar
        winTextRenderer.renderTextScaled("RAPID FIRE", margin, yPos - 2.0f, glm::vec3(1.0f, 0.6f, 0.0f), 1.5f);
        yPos += 18.0f;
        
        // Background bar
        drawHUDQuad(margin, yPos, maxWidth, barHeight, 0.3f, 0.3f, 0.3f);
        // Fill bar (orange pulse)
        float pulse = 0.8f + 0.2f * sin(levelTimer * 10.0f);
        drawHUDQuad(margin, yPos, fillWidth, barHeight, 1.0f * pulse, 0.5f * pulse, 0.0f);
        yPos += barHeight + 10.0f;
    }
    
    // Invincibility power-up bar (when active) - Cyan with name label
    if (hudState.invincibilityActive && hudState.invincibilityTimeRemaining > 0.0f) {
        float maxWidth = 150.0f;
        float fillRatio = hudState.invincibilityTimeRemaining / 2.0f;  // 2 second duration
        float fillWidth = maxWidth * fillRatio;
        
        // Power-up name label above the bar
        winTextRenderer.renderTextScaled("SHIELD", margin, yPos - 2.0f, glm::vec3(0.0f, 0.9f, 1.0f), 1.5f);
        yPos += 18.0f;
        
        // Background bar
        drawHUDQuad(margin, yPos, maxWidth, barHeight, 0.3f, 0.3f, 0.3f);
        // Fill bar (cyan pulse)
        float pulse = 0.8f + 0.2f * sin(levelTimer * 10.0f);
        drawHUDQuad(margin, yPos, fillWidth, barHeight, 0.0f, 0.8f * pulse, 1.0f * pulse);
        yPos += barHeight + 10.0f;
    }
    
    // Portal status bar - Enhanced
    float portalBarWidth = 150.0f;
    float r, g, b;
    std::string portalText;
    
    // Outer border
    drawHUDQuad(margin - 2.0f, yPos - 2.0f, portalBarWidth + 4.0f, barHeight + 4.0f, 0.1f, 0.1f, 0.1f);
    
    if (hudState.portalActive) {
        r = 0.0f; g = 1.0f; b = 0.5f;  // Green (active)
        portalText = "PORTAL READY";
        // Pulsing glow
        float pulse = 0.8f + 0.2f * sin(levelTimer * 6.0f);
        drawHUDQuad(margin - 3.0f, yPos - 3.0f, portalBarWidth + 6.0f, barHeight + 6.0f, 0.0f, 1.0f * 0.4f * pulse, 0.5f * 0.4f * pulse);
        // Gradient fill
        drawHUDQuad(margin, yPos, portalBarWidth, barHeight * 0.5f, 0.2f, 1.0f * pulse, 0.7f * pulse);
        drawHUDQuad(margin, yPos + barHeight * 0.5f, portalBarWidth, barHeight * 0.5f, 0.0f, 0.8f * pulse, 0.4f * pulse);
    } else if (portal && portal->getState() == BlackHoleState::Activating) {
        r = 1.0f; g = 0.5f; b = 0.0f;  // Orange (spawning)
        portalText = "SPAWNING...";
        float pulse = 0.7f + 0.3f * sin(levelTimer * 8.0f);
        drawHUDQuad(margin - 3.0f, yPos - 3.0f, portalBarWidth + 6.0f, barHeight + 6.0f, 1.0f * 0.3f * pulse, 0.5f * 0.3f * pulse, 0.0f);
        drawHUDQuad(margin, yPos, portalBarWidth, barHeight, r * pulse, g * pulse, b * 0.2f);
    } else {
        r = 0.4f; g = 0.4f; b = 0.4f;  // Gray (locked)
        portalText = "LOCKED";
        drawHUDQuad(margin, yPos, portalBarWidth, barHeight, r, g, b);
        // Diagonal stripes for locked state
        for (int i = 0; i < 8; ++i) {
            float stripeX = margin + i * 20.0f;
            drawHUDQuad(stripeX, yPos, 2.0f, barHeight, 0.3f, 0.3f, 0.3f);
        }
    }
    
    // Portal status text
    winTextRenderer.renderTextScaled(portalText, margin + 5.0f, yPos + 6.0f, glm::vec3(r, g, b), 1.0f);
    
    // Score display (top-right corner)
    std::string scoreText = std::to_string(score);
    float scoreX = hudScreenWidth - margin - (scoreText.length() * 12.0f * 2.0f);  // Right-aligned
    winTextRenderer.renderTextScaled(scoreText, scoreX, margin, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f);
    
    // Game Over overlay
    if (!hudState.shipAlive) {
        // Deep space background with more color
        drawHUDQuad(0.0f, 0.0f, hudScreenWidth, hudScreenHeight, 0.05f, 0.05f, 0.15f);
        
        // Animated starfield effect - random stars
        srand(12345);  // Fixed seed for consistent star positions
        for (int i = 0; i < 200; ++i) {
            float starX = (rand() % (int)hudScreenWidth);
            float starY = (rand() % (int)hudScreenHeight);
            float starSize = 1.0f + (rand() % 3);
            
            // Twinkling effect
            float twinkle = 0.5f + 0.5f * sin(levelTimer * 3.0f + i * 0.5f);
            float brightness = 0.7f + 0.3f * twinkle;
            
            drawHUDQuad(starX, starY, starSize, starSize, brightness, brightness, brightness);
        }
        
        // Shooting stars
        for (int i = 0; i < 3; ++i) {
            float shootTime = fmod(levelTimer * 0.8f + i * 2.0f, 5.0f);
            if (shootTime < 1.5f) {
                float progress = shootTime / 1.5f;
                float shootX = -100.0f + progress * (hudScreenWidth + 200.0f);
                float shootY = 100.0f + i * 200.0f + progress * 100.0f;
                
                // Shooting star trail
                for (int j = 0; j < 10; ++j) {
                    float fade = 1.0f - (j / 10.0f);
                    float px = shootX - j * 8.0f;
                    float py = shootY - j * 4.0f;
                    drawHUDQuad(px, py, 4.0f, 2.0f, 0.9f * fade, 0.9f * fade, 1.0f * fade);
                }
            }
        }
        
        // Calculate text positions centered on actual screen dimensions
        float gameOverScale = 3.5f;
        float charWidth = 8.0f * gameOverScale;
        float gameOverTextWidth = 9 * charWidth;
        float gameOverX = (hudScreenWidth - gameOverTextWidth) / 2.0f;
        float gameOverY = (hudScreenHeight / 2.0f) - 60.0f;
        
        // Pulsing cosmic text effect with red/orange glow
        float textPulse = 0.7f + 0.3f * sin(levelTimer * 4.0f);
        glm::vec3 textColor = glm::vec3(1.0f, 0.4f + textPulse * 0.2f, 0.1f);
        
        // Main "GAME OVER" text with glow effect (multiple shadows)
        for (int i = 3; i > 0; --i) {
            float glowDist = i * 2.0f;
            float glowAlpha = 0.4f / i;
            winTextRenderer.renderTextScaled("GAME OVER", gameOverX + glowDist, gameOverY + glowDist, 
                                            glm::vec3(1.0f * glowAlpha, 0.3f * glowAlpha, 0.0f), gameOverScale);
        }
        winTextRenderer.renderTextScaled("GAME OVER", gameOverX, gameOverY, textColor, gameOverScale);
        
        // Score display with cyan cosmic color
        std::string scoreStr = "SCORE: " + std::to_string(score);
        float scoreScale = 2.0f;
        float scoreCharWidth = 8.0f * scoreScale;
        float scoreTextWidth = scoreStr.length() * scoreCharWidth;
        float scoreX = (hudScreenWidth - scoreTextWidth) / 2.0f;
        float scoreY = gameOverY + 80.0f;
        
        winTextRenderer.renderTextScaled(scoreStr, scoreX + 2.0f, scoreY + 2.0f, 
                                        glm::vec3(0.0f, 0.3f, 0.4f), scoreScale);
        winTextRenderer.renderTextScaled(scoreStr, scoreX, scoreY, 
                                        glm::vec3(0.4f, 1.0f, 1.0f), scoreScale);
        
        // Restart instruction with blinking stars effect
        float blinkAlpha = (int)(levelTimer * 2.0f) % 2 == 0 ? 1.0f : 0.6f;
        std::string restartText = "PRESS R TO RESTART";
        float restartScale = 1.5f;
        float restartCharWidth = 8.0f * restartScale;
        float restartTextWidth = restartText.length() * restartCharWidth;
        float restartX = (hudScreenWidth - restartTextWidth) / 2.0f;
        float restartY = scoreY + 70.0f;
        
        winTextRenderer.renderTextScaled(restartText, restartX, restartY, 
                                        glm::vec3(0.8f * blinkAlpha, 0.9f * blinkAlpha, 1.0f * blinkAlpha), restartScale);
    }
    
    // Win overlay (if completed)
    if (completed) {
        // Deep space background - same as game over
        drawHUDQuad(0.0f, 0.0f, hudScreenWidth, hudScreenHeight, 0.05f, 0.05f, 0.15f);
        
        // Animated starfield effect - random stars
        srand(12345);  // Fixed seed for consistent star positions
        for (int i = 0; i < 200; ++i) {
            float starX = (rand() % (int)hudScreenWidth);
            float starY = (rand() % (int)hudScreenHeight);
            float starSize = 1.0f + (rand() % 3);
            
            // Twinkling effect
            float twinkle = 0.5f + 0.5f * sin(levelTimer * 3.0f + i * 0.5f);
            float brightness = 0.7f + 0.3f * twinkle;
            
            drawHUDQuad(starX, starY, starSize, starSize, brightness, brightness, brightness);
        }
        
        // Shooting stars
        for (int i = 0; i < 3; ++i) {
            float shootTime = fmod(levelTimer * 0.8f + i * 2.0f, 5.0f);
            if (shootTime < 1.5f) {
                float progress = shootTime / 1.5f;
                float shootX = -100.0f + progress * (hudScreenWidth + 200.0f);
                float shootY = 100.0f + i * 200.0f + progress * 100.0f;
                
                // Shooting star trail
                for (int j = 0; j < 10; ++j) {
                    float fade = 1.0f - (j / 10.0f);
                    float px = shootX - j * 8.0f;
                    float py = shootY - j * 4.0f;
                    drawHUDQuad(px, py, 4.0f, 2.0f, 0.9f * fade, 0.9f * fade, 1.0f * fade);
                }
            }
        }
        
        // Calculate text positions centered on actual screen dimensions - same as game over
        float winScale = 3.5f;
        float charWidth = 8.0f * winScale;
        float winTextWidth = 9 * charWidth;
        float winX = (hudScreenWidth - winTextWidth) / 2.0f;
        float winY = (hudScreenHeight / 2.0f) - 60.0f;
        
        // Pulsing victory text effect with green glow
        float textPulse = 0.7f + 0.3f * sin(levelTimer * 4.0f);
        glm::vec3 textColor = glm::vec3(0.4f + textPulse * 0.3f, 1.0f, 0.3f + textPulse * 0.2f);
        
        // Main "YOU WIN!!" text with glow effect (multiple shadows)
        for (int i = 3; i > 0; --i) {
            float glowDist = i * 2.0f;
            float glowAlpha = 0.4f / i;
            winTextRenderer.renderTextScaled("YOU WIN!!", winX + glowDist, winY + glowDist, 
                                            glm::vec3(0.0f, 0.6f * glowAlpha, 0.0f), winScale);
        }
        winTextRenderer.renderTextScaled("YOU WIN!!", winX, winY, textColor, winScale);
        
        // Score display with golden color (shifted slightly left)
        std::string scoreStr = "SCORE: " + std::to_string(score);
        float scoreScale = 2.0f;
        float scoreCharWidth = 8.0f * scoreScale;
        float scoreTextWidth = scoreStr.length() * scoreCharWidth;
        float scoreX = (hudScreenWidth - scoreTextWidth) / 2.0f - 40.0f;  // Shift 40 pixels left
        float scoreY = winY + 80.0f;
        
        winTextRenderer.renderTextScaled(scoreStr, scoreX + 2.0f, scoreY + 2.0f, 
                                        glm::vec3(0.4f, 0.3f, 0.0f), scoreScale);
        winTextRenderer.renderTextScaled(scoreStr, scoreX, scoreY, 
                                        glm::vec3(1.0f, 0.9f, 0.2f), scoreScale);
        
        // Continue instruction with blinking effect
        float blinkAlpha = (int)(levelTimer * 2.0f) % 2 == 0 ? 1.0f : 0.6f;
        std::string continueText = "PRESS SPACE TO CONTINUE";
        float continueScale = 1.5f;
        float continueCharWidth = 8.0f * continueScale;
        float continueTextWidth = continueText.length() * continueCharWidth;
        float continueX = (hudScreenWidth - continueTextWidth) / 2.0f;
        float continueY = scoreY + 70.0f;
        
        winTextRenderer.renderTextScaled(continueText, continueX, continueY, 
                                        glm::vec3(0.8f * blinkAlpha, 0.9f * blinkAlpha, 1.0f * blinkAlpha), continueScale);
    }
    
    // Restore OpenGL state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Level2SolarDebrisPath::drawHeart(float x, float y, float size, float r, float g, float b) {
    // Draw a more realistic heart shape with fuller lobes and smooth curves
    
    float halfSize = size / 2.0f;
    float lobeRadius = size * 0.28f;  // Larger lobes for better heart shape
    float centerY = y + size * 0.25f;  // Position lobes higher
    
    int segments = 16;  // More segments for smoother curves
    
    // Left lobe (top-left bump) - draw from bottom to top
    float leftCenterX = x + size * 0.25f;
    for (int i = 0; i < segments; ++i) {
        // Angle from -45° to 225° for a fuller, rounder lobe
        float angle1 = -0.785f + (4.71f * i / segments);
        float angle2 = -0.785f + (4.71f * (i + 1) / segments);
        
        float x1 = leftCenterX + cos(angle1) * lobeRadius;
        float y1 = centerY + sin(angle1) * lobeRadius;
        float x2 = leftCenterX + cos(angle2) * lobeRadius;
        float y2 = centerY + sin(angle2) * lobeRadius;
        
        float vertices[] = {
            leftCenterX, centerY,
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
    
    // Right lobe (top-right bump) - draw from bottom to top
    float rightCenterX = x + size * 0.75f;
    for (int i = 0; i < segments; ++i) {
        float angle1 = -0.785f + (4.71f * i / segments);
        float angle2 = -0.785f + (4.71f * (i + 1) / segments);
        
        float x1 = rightCenterX + cos(angle1) * lobeRadius;
        float y1 = centerY + sin(angle1) * lobeRadius;
        float x2 = rightCenterX + cos(angle2) * lobeRadius;
        float y2 = centerY + sin(angle2) * lobeRadius;
        
        float vertices[] = {
            rightCenterX, centerY,
            x1, y1,
            x2, y2
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        
        glBindVertexArray(hudVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    
    // Fill center area between lobes
    float centerTop = centerY - lobeRadius * 0.5f;
    drawHUDQuad(leftCenterX, centerTop, rightCenterX - leftCenterX, lobeRadius * 0.8f, r, g, b);
    
    // Bottom curved sides leading to the point
    float midY = centerY + lobeRadius * 0.3f;
    int sideCurveSegments = 8;
    
    // Left side curve
    for (int i = 0; i < sideCurveSegments; ++i) {
        float t1 = (float)i / sideCurveSegments;
        float t2 = (float)(i + 1) / sideCurveSegments;
        
        float x1 = x + (1.0f - t1) * size * 0.05f;
        float y1 = midY + t1 * (size - midY);
        float x2 = x + (1.0f - t2) * size * 0.05f;
        float y2 = midY + t2 * (size - midY);
        
        float vertices[] = {
            x + halfSize, y + size,  // Bottom point
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
    
    // Right side curve
    for (int i = 0; i < sideCurveSegments; ++i) {
        float t1 = (float)i / sideCurveSegments;
        float t2 = (float)(i + 1) / sideCurveSegments;
        
        float x1 = x + size - (1.0f - t1) * size * 0.05f;
        float y1 = midY + t1 * (size - midY);
        float x2 = x + size - (1.0f - t2) * size * 0.05f;
        float y2 = midY + t2 * (size - midY);
        
        float vertices[] = {
            x + halfSize, y + size,  // Bottom point
            x2, y2,
            x1, y1
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        
        glUseProgram(hudShaderProgram);
        GLint colorLoc = glGetUniformLocation(hudShaderProgram, "color");
        glUniform3f(colorLoc, r, g, b);
        
        glBindVertexArray(hudVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    
    // Center fill from lobes to point
    float fillTop = midY;
    float fillHeight = size - midY - size * 0.15f;
    drawHUDQuad(leftCenterX, fillTop, rightCenterX - leftCenterX, fillHeight, r, g, b);
    
    glBindVertexArray(0);
}

void Level2SolarDebrisPath::collectAllCells() {
    // Set collected cells to required amount
    collectedCellCount = requiredCellCount;
    hudState.collectedCells = requiredCellCount;
    
    // Activate portal
    if (portal) {
        portal->spawn();  // BlackHolePortal uses spawn() not activate()
        hudState.portalActive = true;
        hudState.portalActivatedFlashTimer = 1.0f;
        std::cout << "[CHEAT] All cells collected! Portal activated!" << std::endl;
    }
}

void Level2SolarDebrisPath::resetCompletionState() {
    completed = false;
    gameOver = false;
    blackHoleFreeze = false;
    portalBeingCaptured = false;
}
