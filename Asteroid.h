#ifndef ASTEROID_H
#define ASTEROID_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Represents a large drift asteroid (non-breakable obstacle)
class Asteroid {
public:
    // Constructor
    Asteroid(const glm::vec3& startPos, const glm::vec3& velocity, float radius);
    ~Asteroid();
    
    // Movement and update
    void update(float dt);
    
    // Rendering
    void setupMesh();
    void render(unsigned int textureID);
    
    // Getters for collision detection
    glm::vec3 getPosition() const { return position; }
    float getRadius() const { return radius; }
    glm::mat4 getModelMatrix() const;
    glm::vec3 getVelocity() const { return velocity; }
    void setVelocity(const glm::vec3& vel) { velocity = vel; }
    
    // Collision detection
    bool checkCollision(const glm::vec3& otherPos, float otherRadius) const;
    
    // Destroy this asteroid
    void destroy() { destroyed = true; }
    bool isDestroyed() const { return destroyed; }
    
    // Cleanup
    void cleanup();
    
    // ON-RAILS: Public position for world scrolling
    glm::vec3 position;
    
private:
    // Transform
    glm::vec3 velocity;     // Movement direction and speed
    glm::vec3 rotation;     // Rotation angles (for visual variety)
    glm::vec3 rotationSpeed; // How fast it rotates on each axis
    
    // Physical properties
    float radius;           // Bounding sphere radius
    float scale;            // Visual scale
    bool destroyed = false; // Whether this asteroid has been destroyed
    
    // Mesh data
    GLuint VAO, VBO, EBO;
    int indexCount;
    
    // Movement bounds (to keep asteroids in play area)
    float movementBoundsX;
    float movementBoundsZ;
    
    void createSphereMesh(int segments);
};

#endif
