#ifndef LEVEL2SOLARDEBRISPATH_H
#define LEVEL2SOLARDEBRISPATH_H

#include "BaseLevel.h"
#include "Skybox.h"
#include "SatelliteFragment.h"
#include "SwingSolarPanel.h"
#include "CometRock.h"
#include "Level2Collectible.h"
#include "BlackHolePortal.h"
#include "Bullet.h"
#include "SimpleTextRenderer.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

/*
 * ═══════════════════════════════════════════════════════════════════════
 *  LEVEL 2 HUD STATE - Real-time feedback for player
 * ═══════════════════════════════════════════════════════════════════════
 * 
 * Tracks all HUD-relevant information for Level 2 display:
 * - Ship health (3 HP system with invulnerability feedback)
 * - Red Energy Cell collection progress (X/6)
 * - Burst Core power-up with remaining time
 * - Portal activation status
 * - Game Over state
 */
struct Level2HUDState {
    // Ship health system
    int shipHealth;
    int shipMaxHealth;
    bool shipInvulnerable;
    bool shipAlive;
    
    // Energy Cell collection progress
    int collectedCells;
    int totalCells;
    
    // Power-up states
    bool rapidFireActive;
    float rapidFireTimeRemaining;
    bool invincibilityActive;
    float invincibilityTimeRemaining;
    
    // Portal state
    bool portalActive;
    
    // Visual feedback timers
    float cellCollectedFlashTimer;
    float powerupStartFlashTimer;
    float portalActivatedFlashTimer;
    
    Level2HUDState()
        : shipHealth(3), shipMaxHealth(3),
          shipInvulnerable(false), shipAlive(true),
          collectedCells(0), totalCells(6),
          rapidFireActive(false), rapidFireTimeRemaining(0.0f),
          invincibilityActive(false), invincibilityTimeRemaining(0.0f),
          portalActive(false),
          cellCollectedFlashTimer(0.0f),
          powerupStartFlashTimer(0.0f),
          portalActivatedFlashTimer(0.0f) {}
};

class Level2SolarDebrisPath : public BaseLevel {
public:
    Level2SolarDebrisPath();
    ~Level2SolarDebrisPath();
    
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
    
    // Check if player controls should be disabled (during black hole freeze)
    bool isControlsDisabled() const { return blackHoleFreeze; }
    
    // Check if rapid fire power-up is active
    bool isRapidFireActive() const { return rapidFireActive; }
    
    // Score persistence
    int getScore() const override { return score; }
    void setScore(int newScore) override { score = newScore; }
    
    // Debug/cheat methods
    void collectAllCells();  // Collect all cells and activate portal
    void resetCompletionState() override;  // Reset completion state
    
private:
    // Skybox for space background
    Skybox* skybox;
    
    // ═══════════════════════════════════════════════════════════════
    //  LEVEL 2 HAZARDS
    // ═══════════════════════════════════════════════════════════════
    
    // Rotating satellite fragments (spinning panels)
    std::vector<SatelliteFragment> satelliteFragments;
    GLuint satelliteTexture;
    
    // Swinging solar panels (pendulum obstacles)
    std::vector<SwingSolarPanel> solarPanels;
    GLuint solarPanelTexture;
    
    // Fast breakable comet rocks
    std::vector<CometRock> cometRocks;
    GLuint cometTexture;
    
    // Bullets
    std::vector<Bullet> bullets;
    GLuint bulletTexture;
    
    // ═══════════════════════════════════════════════════════════════
    //  LEVEL 2 COLLECTIBLES
    // ═══════════════════════════════════════════════════════════════
    
    // Red Energy Cells and Burst Core power-ups
    std::vector<Level2Collectible> collectibles;
    GLuint collectibleTexture;
    int collectedCellCount;
    int requiredCellCount;
    
    // Collectible spawn system
    static const int L2_REQUIRED_CELLS = 6;
    static const int L2_MAX_CELLS_TO_SPAWN = 10;
    static const int L2_MAX_POWERUPS_TO_SPAWN = 2;
    int spawnedCells;
    int spawnedPowerups;
    float distanceTraveled;
    float nextCollectibleSpawnDistance;
    
    // ═══════════════════════════════════════════════════════════════
    //  BLACK HOLE PORTAL (end goal for Level 2)
    // ═══════════════════════════════════════════════════════════════
    
    BlackHolePortal* portal;
    GLuint portalTexture;
    bool portalBeingCaptured;
    
    // ═══════════════════════════════════════════════════════════════
    //  POWER-UP STATES
    // ═══════════════════════════════════════════════════════════════
    
    bool rapidFireActive;
    float rapidFireEndTime;
    bool invincibilityActive;
    float invincibilityEndTime;
    float powerupDuration;  // 8 seconds for L2 power-ups
    
    // Shield bubble mesh for invincibility visual
    GLuint shieldBubbleVAO;
    GLuint shieldBubbleVBO;
    GLuint shieldBubbleEBO;
    int shieldBubbleIndexCount;
    void createShieldBubbleMesh();
    
    // ═══════════════════════════════════════════════════════════════
    //  ON-RAILS SCROLLING SYSTEM
    // ═══════════════════════════════════════════════════════════════
    
    float scrollSpeed;
    float worldZOffset;
    
    // Spawn system constants
    static const int L2_MAX_SATELLITE_FRAGMENTS = 15;
    static const int L2_MAX_SOLAR_PANELS = 8;
    static const int L2_MAX_COMET_ROCKS = 25;
    static const float L2_PLAY_AREA_MIN_X;
    static const float L2_PLAY_AREA_MAX_X;
    static const float L2_PLAY_AREA_MIN_Y;
    static const float L2_PLAY_AREA_MAX_Y;
    static const float L2_SPAWN_DISTANCE_AHEAD;
    
    // ═══════════════════════════════════════════════════════════════
    //  LEVEL STATE
    // ═══════════════════════════════════════════════════════════════
    
    bool completed;
    bool gameOver;
    bool blackHoleFreeze;  // When true, player is frozen inside black hole with black screen
    float levelTimer;
    float lastFireTime;
    float fireRate;
    float burstFireRate;  // Faster fire rate when Burst Core is active
    int score;            // Player score (comets = 10, collectibles = 200)
    
    // HUD state
    Level2HUDState hudState;
    
    // Screen dimensions for HUD centering
    int hudScreenWidth;
    int hudScreenHeight;
    
    // Win screen text renderer
    SimpleTextRenderer winTextRenderer;
    
    // Key state tracking for debouncing
    bool spaceKeyWasPressed;
    
    // ═══════════════════════════════════════════════════════════════
    //  2D HUD RENDERING
    // ═══════════════════════════════════════════════════════════════
    
    GLuint hudVAO;
    GLuint hudVBO;
    GLuint hudShaderProgram;
    
    // ═══════════════════════════════════════════════════════════════
    //  PRIVATE METHODS
    // ═══════════════════════════════════════════════════════════════
    
    void initSatelliteFragments();
    void initSolarPanels();
    void initCometRocks();
    void initCollectibles();
    void initPortal();
    
    void updateSatelliteSpawning(float cameraZ);
    void updateSolarPanelSpawning(float cameraZ);
    void updateCometSpawning(float cameraZ);
    void updateCollectibleSpawning();
    
    void checkCollisions(Game& game);
    void checkBulletCollisions();
    void checkCollectibleCollisions(Game& game);
    void checkHazardCollisions(Game& game);
    
    void updateHUD(float dt, Game& game);
    void resetLevel(Game& game);
    
    // HUD rendering methods
    void setupHUD2D(int screenWidth, int screenHeight);
    void cleanupHUD2D();
    void renderHUD(Game& game);
    void renderHUD2D();
    void drawHUDQuad(float x, float y, float width, float height, float r, float g, float b);
    void drawHeart(float x, float y, float size, float r, float g, float b);
    void drawText(const char* text, float x, float y, float scale, float r, float g, float b);
};

#endif
