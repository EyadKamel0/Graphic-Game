#ifndef SMALLASTEROID_H
#define SMALLASTEROID_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Represents a small breakable asteroid that can be destroyed by bullets
class SmallAsteroid {
public:
    enum class State {
        Normal,      // Floating normally
        Breaking,    // Hit by bullet, animating destruction
        Dead         // Fully destroyed, ready for removal
    };
    
    SmallAsteroid(const glm::vec3& startPos, const glm::vec3& velocity, float radius = 1.0f);
    ~SmallAsteroid();
    
    void update(float dt);
    void render(unsigned int textureID);
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    float getRadius() const { return radius; }
    State getState() const { return state; }
    bool isDead() const { return state == State::Dead; }
    glm::mat4 getModelMatrix() const;
    glm::vec3 getVelocity() const { return velocity; }
    
    // Collision
    bool checkCollision(const glm::vec3& bulletPos, float bulletRadius) const;
    
    // Trigger destruction sequence
    void startBreaking();
    
    // Setters for respawning
    void setVelocity(const glm::vec3& vel) { velocity = vel; }
    void respawn(const glm::vec3& newPos, const glm::vec3& newVel, float newRadius);
    
    void cleanup();
    
    // ON-RAILS: Public position for world scrolling
    glm::vec3 position;
    
private:
    glm::vec3 velocity;
    glm::vec3 rotation;
    glm::vec3 rotationSpeed;
    
    float radius;
    float scale;
    State state;
    
    // Breaking animation state
    float breakTimer;           // Counts up during breaking animation
    float breakDuration;        // How long the break animation lasts
    float initialScale;
    
    // Mesh
    GLuint VAO, VBO, EBO;
    int indexCount;
    
    void setupMesh();
    void createSphereMesh(int segments);
};

// Fragment that flies away from a broken asteroid
class AsteroidFragment {
public:
    AsteroidFragment(const glm::vec3& startPos, const glm::vec3& velocity, float size);
    ~AsteroidFragment();
    
    // Delete copy operations (OpenGL resources can't be safely copied)
    AsteroidFragment(const AsteroidFragment&) = delete;
    AsteroidFragment& operator=(const AsteroidFragment&) = delete;
    
    // Move operations (transfer ownership of OpenGL resources)
    AsteroidFragment(AsteroidFragment&& other) noexcept;
    AsteroidFragment& operator=(AsteroidFragment&& other) noexcept;
    
    void update(float dt);
    void render(unsigned int textureID);
    
    bool isDead() const { return lifetime <= 0.0f; }
    float getAlpha() const { return lifetime / maxLifetime; }
    glm::mat4 getModelMatrix() const;
    
    void cleanup();
    
    // ON-RAILS: Public position for world scrolling
    glm::vec3 position;
    
private:
    glm::vec3 velocity;
    glm::vec3 rotation;
    glm::vec3 rotationSpeed;
    
    float size;
    float lifetime;
    float maxLifetime;
    
    GLuint VAO, VBO;
    
    void setupMesh();
};

#endif
