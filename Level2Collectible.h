#ifndef LEVEL2COLLECTIBLE_H
#define LEVEL2COLLECTIBLE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

/*
 * ═══════════════════════════════════════════════════════════════════════
 *  LEVEL 2 COLLECTIBLES - Red Energy Cells & Burst Core Power-Up
 * ═══════════════════════════════════════════════════════════════════════
 * 
 * Red Energy Cells: Required to open the final portal (uses 1_1 texture)
 * Burst Core Power-Up: Grants rapid-fire shooting (uses 1_3 texture)
 */

enum class Level2CollectibleType {
    RedEnergyCell,   // Required collectible to open portal (1_1 texture)
    RapidFire,       // Power-up: no shot cooldown for 2 seconds (1_3 texture)
    Invincibility    // Power-up: shield bubble for 2 seconds (1_3 texture)
};

// Shared mesh data structure
struct Level2CollectibleSubMesh {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    int indexCount = 0;
    GLuint texture = 0;
};

class Level2Collectible {
public:
    Level2Collectible(const glm::vec3& startPos, Level2CollectibleType type);
    ~Level2Collectible();
    
    // Disable copy to prevent VAO/VBO/EBO handle duplication
    Level2Collectible(const Level2Collectible&) = delete;
    Level2Collectible& operator=(const Level2Collectible&) = delete;
    
    // Move constructor and assignment
    Level2Collectible(Level2Collectible&& other) noexcept;
    Level2Collectible& operator=(Level2Collectible&& other) noexcept;
    
    void update(float dt);
    void render(unsigned int textureID);  // textureID ignored, uses own texture
    
    // Static mesh loading
    static void loadSharedMeshes();
    static void cleanupSharedMeshes();
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    float getRadius() const { return radius; }
    Level2CollectibleType getType() const { return type; }
    bool isCollected() const { return collected; }
    bool isReadyToRemove() const { return collected && pickupAnimTimer >= pickupAnimDuration; }
    glm::mat4 getModelMatrix() const;
    float getGlowIntensity() const { return glowIntensity; }
    glm::vec3 getGlowColor() const;
    
    // Collision with player
    bool checkCollision(const glm::vec3& playerPos, float playerRadius);
    void collect();
    void respawn(const glm::vec3& newPos);
    
    // Force movement (for black hole suction effect)
    void forceMove(const glm::vec3& delta) { position += delta; }
    
    void cleanup();
    void setupMesh();
    
    // ON-RAILS: Public position for world scrolling
    glm::vec3 position;
    
private:
    glm::vec3 rotation;
    glm::vec3 rotationSpeed;
    
    Level2CollectibleType type;
    float radius;
    float scale;
    bool collected;
    
    // Bobbing/floating animation
    float bobTimer;
    float bobSpeed;
    float bobAmount;
    float originalY;
    
    // Pulsing glow effect
    float glowIntensity;
    float glowPulseTimer;
    float glowPulseSpeed;
    float glowMin;
    float glowMax;
    
    // Pickup animation
    float pickupAnimTimer;
    float pickupAnimDuration;
    
    // Instance mesh (fallback)
    GLuint VAO, VBO, EBO;
    int indexCount;
    bool usesSharedMesh;
    
    // Static shared meshes
    static Level2CollectibleSubMesh sharedMesh_1_1;  // Red energy cells
    static Level2CollectibleSubMesh sharedMesh_1_3;  // Power-ups
    static bool sharedMeshesLoaded;
    
    static void loadOBJWithTexture(const std::string& objPath, const std::string& texturePath, Level2CollectibleSubMesh& outMesh);
    void createFallbackMesh();
    void playPickupSound();
};

#endif
