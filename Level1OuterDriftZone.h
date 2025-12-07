#ifndef LEVEL1OUTERDRIFTZONE_H
#define LEVEL1OUTERDRIFTZONE_H

#include "BaseLevel.h"
#include "Asteroid.h"
#include "SmallAsteroid.h"
#include "Bullet.h"
#include "Collectible.h"
#include "WavePortal.h"
#include "Skybox.h"
#include "SimpleTextRenderer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

/*
 * ═══════════════════════════════════════════════════════════════════════
 *  LEVEL 1 HUD STATE - Real-time feedback for player
 * ═══════════════════════════════════════════════════════════════════════
 * 
 * Tracks all HUD-relevant information for Level 1 display:
 * - Ship health (3 HP system with invulnerability feedback)
 * - Energy Shard collection progress (X/5)
 * - Active power-ups with remaining time
 * - Portal activation status
 * - Visual feedback timers (flash effects)
 * - Game Over state
 */
struct Level1HUDState {
    // Ship health system
    int shipHealth;
    int shipMaxHealth;
    bool shipInvulnerable;
    bool shipAlive;
    
    // Shard collection progress
    int collectedShards;
    int totalShards;
    
    // Power-up states
    bool jetBoosterActive;
    float jetBoosterTimeRemaining;  // Seconds remaining (rounded for display)
    bool miniBurstActive;
    float miniBurstTimeRemaining;   // Seconds remaining (rounded for display)
    
    // Portal state
    bool portalActive;
    
    // Visual feedback timers (for flash/highlight effects)
    float shardCollectedFlashTimer;    // >0 = flash shard count (lasts 0.3s)
    float powerupStartFlashTimer;      // >0 = flash power-up line (lasts 0.5s)
    float portalActivatedFlashTimer;   // >0 = flash portal text (lasts 1.0s)
    
    Level1HUDState()
        : shipHealth(3), shipMaxHealth(3),
          shipInvulnerable(false), shipAlive(true),
          collectedShards(0), totalShards(5),
          jetBoosterActive(false), jetBoosterTimeRemaining(0.0f),
          miniBurstActive(false), miniBurstTimeRemaining(0.0f),
          portalActive(false),
          shardCollectedFlashTimer(0.0f),
          powerupStartFlashTimer(0.0f),
          portalActivatedFlashTimer(0.0f) {}
};

class Level1OuterDriftZone : public BaseLevel {
public:
    Level1OuterDriftZone();
    ~Level1OuterDriftZone();
    
    void init(Game& game) override;
    void update(float dt, Game& game) override;
    void render(Game& game) override;
    bool isCompleted() const override;
    std::string getLevelName() const override;
    LevelType getLevelType() const override;
    void cleanup() override;
    
    glm::vec3 getPlayerSpawnPosition() const override;
    glm::vec3 getPlayerSpawnRotation() const override;
    
    // Shooting mechanic
    void fireBullet(const glm::vec3& position, const glm::vec3& direction);
    
    // Score persistence
    int getScore() const override { return score; }
    void setScore(int newScore) override { score = newScore; }
    
    // Debug/cheat methods
    void collectAllShards();  // Collect all shards and activate portal
    void resetCompletionState() override;  // Reset completion state
    
private:
    // Skybox for deep space background
    Skybox* skybox;
    
    // Level-specific geometry
    GLuint planeVAO, planeVBO;
    GLuint planeTexture;
    int planeVertexCount;  // Number of vertices in the grid
    static const bool L1_RENDER_FLOOR = false;  // Disable floor grid rendering
    
    // Large drift asteroids (non-breakable)
    std::vector<Asteroid> asteroids;
    GLuint asteroidTexture;
    
    // Small breakable asteroids
    std::vector<SmallAsteroid> smallAsteroids;
    GLuint smallAsteroidTexture;
    
    // Bullets
    std::vector<Bullet> bullets;
    GLuint bulletTexture;
    
    // Fragments from destroyed asteroids
    std::vector<AsteroidFragment> fragments;
    
    // Collectibles - power-ups and score items
    std::vector<Collectible> collectibles;
    GLuint collectibleTexture;
    int collectedShardCount;
    int requiredShardCount;  // Number of shards needed to activate portal
    
    // Infinite spawn system for collectibles
    static const int L1_REQUIRED_SHARDS = 5;        // How many shards needed to activate portal
    static const int L1_MAX_SHARDS_TO_SPAWN = 14;   // Total possible shard spawns (+16.7%)
    static const int L1_MAX_POWERUPS_TO_SPAWN = 3;  // Total power-ups to spawn
    int spawnedShards;       // Counts shards spawned so far
    int spawnedPowerups;     // Counts power-ups spawned so far
    float distanceTraveled;
    float nextCollectibleSpawnDistance;
    
    // Wave Portal - level goal
    WavePortal* portal;
    GLuint portalTexture;
    bool portalBeingCaptured;  // True when player is being sucked in
    
    // Power-up states
    bool jetBoosterActive;
    float jetBoosterEndTime;
    bool miniBurstActive;
    float miniBurstEndTime;
    float powerupDuration;  // How long power-ups last
    
    // ON-RAILS SCROLLING - World moves toward player like classic arcade
    float scrollSpeed;          // Speed at which world moves toward player (along +Z)
    float worldZOffset;         // Current Z offset for scrolling effect
    
    // Infinite asteroid spawn system constants
    static const int L1_MAX_LARGE_ASTEROIDS = 40;  // 70% of total asteroids
    static const int L1_MAX_SMALL_ASTEROIDS = 17;  // 30% of total asteroids
    static const float L1_PLAY_AREA_MIN_X;
    static const float L1_PLAY_AREA_MAX_X;
    static const float L1_PLAY_AREA_MIN_Y;
    static const float L1_PLAY_AREA_MAX_Y;
    static const float L1_SPAWN_DISTANCE_AHEAD;  // How far ahead to spawn
    
    // Level state
    bool completed;
    bool gameOver;      // Game Over state (ship destroyed, health = 0)
    float levelTimer;   // Track time in level for completion condition
    float lastFireTime; // For fire rate limiting
    float fireRate;     // Minimum time between shots
    int score;          // Player score (small asteroids = 5, collectibles = 100)
    
    // Screen dimensions for HUD centering
    int hudScreenWidth;
    int hudScreenHeight;
    
    // HUD system for Level 1
    Level1HUDState hudState;
    
    // Win screen text renderer (also used for power-up labels)
    SimpleTextRenderer winTextRenderer;
    
    void setupPlane();
    void setupAsteroids();      // Legacy - replaced by spawn system
    void setupSmallAsteroids(); // Legacy - replaced by spawn system
    void setupCollectibles();   // Legacy - replaced by spawn system
    void initAsteroidSpawner();      // Initialize infinite asteroid spawn
    void initSmallAsteroidSpawner(); // Initialize infinite small asteroid spawn
    void initCollectibleSpawner();   // Initialize distance-based collectible spawn
    void updateAsteroidSpawning(float cameraZ);       // Recycle and spawn asteroids
    void updateSmallAsteroidSpawning(float cameraZ);  // Recycle and spawn small asteroids
    void updateCollectibleSpawning();  // Spawn collectibles based on distance
    void resetLevel(Game& game);  // Reset Level 1 state for restart (press R)
    void checkCollisions(Game& game);
    void checkBulletCollisions();
    void checkCollectibleCollisions(Game& game);
    void checkAsteroidCollisions();  // Asteroid-asteroid collision
    
    // Collision helper - swept sphere collision detection
    bool checkSweptCollision(const glm::vec3& start, const glm::vec3& end, 
                             float radius1, const glm::vec3& targetPos, float radius2);
    
    // HUD management
    void updateHUD(float dt, Game& game);
    void renderHUD(Game& game);
    void renderHUD2D();  // Simple 2D orthographic HUD
    
    // 2D HUD rendering helpers
    GLuint hudVAO, hudVBO;
    GLuint hudShaderProgram;
    void setupHUD2D(int screenWidth, int screenHeight);
    void cleanupHUD2D();
    void drawHUDQuad(float x, float y, float width, float height, float r, float g, float b);
    void drawHeart(float x, float y, float size, float r, float g, float b);
    
    // Audio placeholder
    void playAsteroidBreakSound();
};

#endif
