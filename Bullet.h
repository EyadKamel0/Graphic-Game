#ifndef BULLET_H
#define BULLET_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Represents a projectile fired by the player ship
class Bullet {
public:
    Bullet(const glm::vec3& startPos, const glm::vec3& direction, float speed = 50.0f);
    ~Bullet();
    
    // Disable copy to prevent VAO/VBO handle duplication
    Bullet(const Bullet&) = delete;
    Bullet& operator=(const Bullet&) = delete;
    
    // Move constructor and assignment
    Bullet(Bullet&& other) noexcept;
    Bullet& operator=(Bullet&& other) noexcept;
    
    void update(float dt);
    void render(unsigned int textureID);
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getPreviousPosition() const { return previousPosition; }
    float getRadius() const { return radius; }
    bool isExpired() const { return lifetime <= 0.0f || shouldDestroy; }
    glm::mat4 getModelMatrix() const;
    
    // Collision detection
    bool checkCollision(const glm::vec3& targetPos, float targetRadius) const;
    
    // Mark for destruction (used during collision detection)
    void markForDestruction() { shouldDestroy = true; }
    
    // Pull toward a position (for black hole effect)
    void pullToward(const glm::vec3& target, float amount) {
        glm::vec3 dir = glm::normalize(target - position);
        position += dir * amount;
    }
    
    void cleanup();
    
private:
    glm::vec3 position;
    glm::vec3 previousPosition;  // For continuous collision detection
    glm::vec3 velocity;
    float speed;
    float lifetime;         // Time until bullet disappears (seconds)
    float maxLifetime;
    float radius;           // Collision radius
    bool shouldDestroy;     // Flag for safe removal during collision
    
    // Mesh data (simple small cube/sphere for visual)
    GLuint VAO, VBO;
    
    void setupMesh();
};

#endif
