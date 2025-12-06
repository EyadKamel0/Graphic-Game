#ifndef COLLECTIBLE_H
#define COLLECTIBLE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

// Types of collectibles in the game
enum class CollectibleType {
    EnergyShard,    // Required collectible to open portal (uses 1_2 texture)
    JetBooster,     // Power-up: temporary speed boost (uses 1_3 texture)
    MiniBurstShot   // Power-up: spread shot (uses 1_3 texture)
};

// Shared mesh data structure
struct CollectibleSubMesh {
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    int indexCount = 0;
    GLuint texture = 0;
};

// Represents a collectible item floating in space
class Collectible {
public:
    Collectible(const glm::vec3& startPos, CollectibleType type);
    ~Collectible();
    
    // Disable copy to prevent VAO/VBO/EBO handle duplication
    Collectible(const Collectible&) = delete;
    Collectible& operator=(const Collectible&) = delete;
    
    // Move constructor and assignment
    Collectible(Collectible&& other) noexcept;
    Collectible& operator=(Collectible&& other) noexcept;
    
    void update(float dt);
    void render(unsigned int textureID);  // textureID ignored, uses own texture
    
    // Static mesh loading
    static void loadSharedMeshes();
    static void cleanupSharedMeshes();
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    float getRadius() const { return radius; }
    CollectibleType getType() const { return type; }
    bool isCollected() const { return collected; }
    bool isReadyToRemove() const { return collected && pickupAnimTimer >= pickupAnimDuration; }
    glm::mat4 getModelMatrix() const;
    
    // Collision with player
    bool checkCollision(const glm::vec3& playerPos, float playerRadius);
    void collect();  // Mark as collected and start pickup animation
    void respawn(const glm::vec3& newPos);  // Respawn at a new position (for recycling missed collectibles)
    
    void cleanup();
    void setupMesh();  // Initialize OpenGL buffers (can be called explicitly or lazy-loaded)
    
    // ON-RAILS: Public position for world scrolling
    glm::vec3 position;
    
private:
    glm::vec3 rotation;
    glm::vec3 rotationSpeed;
    
    CollectibleType type;
    float radius;
    float scale;
    bool collected;
    
    // Bobbing/floating animation
    float bobTimer;
    float bobSpeed;
    float bobAmount;
    float originalY;
    
    // Pickup animation (scale down + glow)
    float pickupAnimTimer;
    float pickupAnimDuration;
    float emissiveIntensity;  // Glow effect on pickup
    
    // Instance mesh (fallback)
    GLuint VAO, VBO, EBO;
    int indexCount;
    bool usesSharedMesh;
    
    // Static shared meshes
    static CollectibleSubMesh sharedMesh_1_2;  // Energy shards
    static CollectibleSubMesh sharedMesh_1_3;  // Power-ups
    static bool sharedMeshesLoaded;
    
    static void loadOBJWithTexture(const std::string& objPath, const std::string& texturePath, CollectibleSubMesh& outMesh);
    void createFallbackMesh();
    void playPickupSound();
};

#endif
