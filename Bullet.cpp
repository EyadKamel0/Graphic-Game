#include "Bullet.h"
#include <glad/glad.h>
#include <iostream>

/*
 * BULLET DESPAWN SETTINGS
 * 
 * Bullets automatically despawn after traveling a certain distance.
 * This prevents them from flying forever and cluttering the scene.
 * 
 * In the fake 3D on-rails setup, bullets always shoot straight ahead (-Z direction).
 */
const float BULLET_MAX_LIFETIME = 2.0f;  // Seconds before bullet despawns

Bullet::Bullet(const glm::vec3& startPos, const glm::vec3& direction, float spd)
    : position(startPos), previousPosition(startPos), speed(spd), lifetime(BULLET_MAX_LIFETIME), 
      maxLifetime(BULLET_MAX_LIFETIME), radius(0.2f), shouldDestroy(false), VAO(0), VBO(0) {
    
    velocity = glm::normalize(direction) * speed;
    // Don't call setupMesh here - will be called on first render if needed
}

Bullet::~Bullet() {
    cleanup();
}

// Move constructor
Bullet::Bullet(Bullet&& other) noexcept
    : position(other.position),
      previousPosition(other.previousPosition),
      velocity(other.velocity),
      speed(other.speed),
      lifetime(other.lifetime),
      maxLifetime(other.maxLifetime),
      radius(other.radius),
      shouldDestroy(other.shouldDestroy),
      VAO(other.VAO),
      VBO(other.VBO) {
    // Take ownership of OpenGL resources
    other.VAO = 0;
    other.VBO = 0;
}

// Move assignment operator
Bullet& Bullet::operator=(Bullet&& other) noexcept {
    if (this != &other) {
        // Clean up existing resources
        // Move data
        position = other.position;
        previousPosition = other.previousPosition;
        velocity = other.velocity;
        speed = other.speed;
        lifetime = other.lifetime;
        maxLifetime = other.maxLifetime;
        radius = other.radius;
        shouldDestroy = other.shouldDestroy;
        VAO = other.VAO;
        VBO = other.VBO;
        VBO = other.VBO;
        VBO = other.VBO;
        
        // Take ownership
        other.VAO = 0;
        other.VBO = 0;
    }
    return *this;
}

void Bullet::setupMesh() {
    if (VAO != 0) return;  // Already set up
    
    // Simple small cube for the bullet projectile
    float size = 0.15f;
    float vertices[] = {
        // Position (x,y,z), Normal (nx,ny,nz), TexCoord (u,v)
        // Front face
        -size, -size,  size,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         size, -size,  size,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         size,  size,  size,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         size,  size,  size,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -size,  size,  size,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -size, -size,  size,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        
        // Back face
        -size, -size, -size,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        -size,  size, -size,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         size,  size, -size,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         size,  size, -size,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         size, -size, -size,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -size, -size, -size,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        
        // Left face
        -size,  size,  size, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -size,  size, -size, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -size, -size, -size, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -size, -size, -size, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -size, -size,  size, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -size,  size,  size, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        
        // Right face
         size,  size,  size,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         size, -size,  size,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
         size, -size, -size,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         size, -size, -size,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         size,  size, -size,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         size,  size,  size,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        
        // Top face
        -size,  size, -size,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -size,  size,  size,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
         size,  size,  size,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         size,  size,  size,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         size,  size, -size,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
        -size,  size, -size,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        
        // Bottom face
        -size, -size, -size,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         size, -size, -size,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         size, -size,  size,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         size, -size,  size,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
        -size, -size,  size,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
        -size, -size, -size,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // TexCoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void Bullet::update(float dt) {
    /*
     * BULLET UPDATE & DESPAWN LOGIC
     * 
     * Bullets move forward (in the -Z direction for the on-rails setup)
     * and their lifetime decreases each frame.
     * 
     * When lifetime reaches 0, isExpired() returns true and the bullet
     * is removed from the game by the level's update loop.
     * 
     * Current settings:
     * - Speed: ~50 units/sec (set when bullet is created)
     * - Max lifetime: 5 seconds
     * - Max distance traveled: ~250 units
     */
    
    // Store previous position for continuous collision detection
    previousPosition = position;
    
    position += velocity * dt;
    lifetime -= dt;
}

glm::mat4 Bullet::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    return model;
}

void Bullet::render(unsigned int textureID) {
    // Lazy initialization - set up mesh on first render
    if (VAO == 0) {
        setupMesh();
        // Safety check after setup
        if (VAO == 0) {
            std::cerr << "[ERROR] Bullet mesh setup failed!" << std::endl;
            return;
        }
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

bool Bullet::checkCollision(const glm::vec3& targetPos, float targetRadius) const {
    float distance = glm::length(position - targetPos);
    return distance < (radius + targetRadius);
}

// Continuous collision detection - swept sphere against static sphere
static bool checkSweptSphereCollision(const glm::vec3& sphereStart, const glm::vec3& sphereEnd, 
                                       float sphereRadius, const glm::vec3& targetPos, float targetRadius) {
    // Ray from start to end
    glm::vec3 ray = sphereEnd - sphereStart;
    float rayLength = glm::length(ray);
    
    if (rayLength < 0.0001f) {
        // No movement, use simple sphere test
        float dist = glm::length(sphereStart - targetPos);
        return dist < (sphereRadius + targetRadius);
    }
    
    glm::vec3 rayDir = ray / rayLength;
    
    // Vector from ray start to sphere center
    glm::vec3 toSphere = targetPos - sphereStart;
    
    // Project onto ray
    float projection = glm::dot(toSphere, rayDir);
    
    // Clamp to ray segment
    projection = glm::clamp(projection, 0.0f, rayLength);
    
    // Closest point on ray to sphere center
    glm::vec3 closestPoint = sphereStart + rayDir * projection;
    
    // Distance from closest point to sphere center
    float distanceToCenter = glm::length(closestPoint - targetPos);
    float combinedRadius = sphereRadius + targetRadius;
    
    return distanceToCenter < combinedRadius;
}

void Bullet::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
}
