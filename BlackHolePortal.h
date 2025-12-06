#ifndef BLACKHOLEPORTAL_H
#define BLACKHOLEPORTAL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Portal lifecycle states
enum class BlackHoleState {
    Hidden,      // Not visible yet
    Activating,  // Spawning/growing animation
    Active,      // Ready to enter
    Capturing,   // Pulling player in
    Completed    // Player fully absorbed
};

// Black Hole Portal - End goal for Level 2
class BlackHolePortal {
public:
    BlackHolePortal(const glm::vec3& portalPos);
    ~BlackHolePortal();
    
    void update(float dt);
    void render(unsigned int shaderProgram);
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    float getRadius() const { return triggerRadius; }
    bool isActive() const { return state == BlackHoleState::Active; }
    BlackHoleState getState() const { return state; }
    glm::mat4 getModelMatrix() const;
    
    // Lifecycle control
    void spawn();  // Begin spawn animation
    void startCapture();  // Begin pulling player in
    bool checkPlayerEntry(const glm::vec3& playerPos, float playerRadius);
    
    // Capture effect
    glm::vec3 pullPlayerToward(const glm::vec3& playerPos, float dt);
    
    void cleanup();
    void setupMesh();
    
    // ON-RAILS: Public position for world scrolling
    glm::vec3 position;
    
private:
    BlackHoleState state;
    
    // Ring rotations
    float ringRotation1;
    float ringRotation2;
    float ringRotation3;
    float ringSpeed1;
    float ringSpeed2;
    float ringSpeed3;
    
    float scale;
    float targetScale;
    float triggerRadius;
    
    // Spawn animation
    float spawnAnimTimer;
    float spawnAnimDuration;
    
    // Capture animation
    float captureTimer;
    float captureDuration;
    float captureStrength;
    
    // Visual effects
    float pulseTimer;
    float pulseSpeed;
    float glowIntensity;
    
    // Mesh data - Core (dark center)
    GLuint coreVAO, coreVBO, coreEBO;
    int coreIndexCount;
    
    // Mesh data - Ring 1 (main accretion disk)
    GLuint ring1VAO, ring1VBO, ring1EBO;
    int ring1IndexCount;
    
    // Mesh data - Ring 2 (outer ring)
    GLuint ring2VAO, ring2VBO, ring2EBO;
    int ring2IndexCount;
    
    // Mesh data - Light glow
    GLuint glowVAO, glowVBO, glowEBO;
    int glowIndexCount;
    
    // Textures
    GLuint ringTexture;
    GLuint lightTexture1;
    GLuint lightTexture2;
    GLuint lightTexture3;
    
    void createCoreMesh();
    void createRingMesh(GLuint& vao, GLuint& vbo, GLuint& ebo, int& indexCount, float innerRadius, float outerRadius);
    void createGlowMesh();
    void loadTextures();
};

#endif
