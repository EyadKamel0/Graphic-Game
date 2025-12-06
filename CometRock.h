#ifndef COMETROCK_H
#define COMETROCK_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

/*
 * ═══════════════════════════════════════════════════════════════════════
 *  COMET ROCK - Level 2 Fast Breakable Hazard
 * ═══════════════════════════════════════════════════════════════════════
 * 
 * Sci-Fi debris objects that are destructible by player shots.
 * Uses the Cylinder_Sci_Fi_1 model with multi-material textures.
 */
class CometRock {
public:
    enum class State {
        Normal,      // Floating in space
        Breaking,    // Hit by bullet, animating destruction
        Dead         // Fully destroyed, ready for removal
    };
    
    // Submesh for multi-material rendering
    struct Submesh {
        GLuint VAO = 0;
        GLuint VBO = 0;
        GLuint EBO = 0;
        int indexCount = 0;
        std::string materialName;
    };
    
    // Material with textures
    struct Material {
        std::string name;
        GLuint baseColorTexture = 0;
    };
    
    CometRock(const glm::vec3& startPos, const glm::vec3& velocity, float radius = 2.0f);
    ~CometRock();
    
    // Disable copy to prevent VAO/VBO/EBO handle duplication
    CometRock(const CometRock&) = delete;
    CometRock& operator=(const CometRock&) = delete;
    
    // Move constructor and assignment
    CometRock(CometRock&& other) noexcept;
    CometRock& operator=(CometRock&& other) noexcept;
    
    void update(float dt);
    void render(unsigned int fallbackTexture);
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    float getRadius() const { return radius; }
    float getCollisionRadius() const { return collisionRadius; }
    State getState() const { return state; }
    bool isDead() const { return state == State::Dead; }
    glm::mat4 getModelMatrix() const;
    glm::vec3 getVelocity() const { return velocity; }
    
    // Collision detection
    bool checkCollision(const glm::vec3& bulletPos, float bulletRadius) const;
    bool checkPlayerCollision(const glm::vec3& playerPos, float playerRadius) const;
    
    // Trigger destruction sequence
    void startBreaking();
    
    // Immediate destruction (for black hole effect)
    void destroy() { state = State::Dead; }
    
    // Move toward a position (for black hole suction)
    void moveToward(const glm::vec3& target, float amount) {
        glm::vec3 dir = glm::normalize(target - position);
        position += dir * amount;
    }
    
    // Setters for respawning
    void setVelocity(const glm::vec3& vel) { velocity = vel; }
    void respawn(const glm::vec3& newPos, const glm::vec3& newVel, float newRadius = 2.0f);
    
    void cleanup();
    
    // ON-RAILS: Public position for world scrolling
    glm::vec3 position;
    
    // Static model data (shared across all instances)
    static std::vector<Submesh> sharedSubmeshes;
    static std::vector<Material> sharedMaterials;
    static bool modelLoaded;
    static void loadModel();
    static void cleanupSharedResources();
    
private:
    glm::vec3 velocity;
    glm::vec3 previousPosition;  // For swept collision detection
    glm::vec3 rotation;
    glm::vec3 rotationSpeed;
    
    float radius;
    float collisionRadius;
    float scale;
    State state;
    
    // Breaking animation state
    float breakTimer;
    float breakDuration;
    float initialScale;
};

#endif
