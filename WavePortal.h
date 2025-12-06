#ifndef WAVEPORTAL_H
#define WAVEPORTAL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Portal lifecycle states
enum class PortalState {
    Hidden,      // Not spawned yet (before all shards collected)
    Activating,  // Spawning/growing animation
    Active,      // Ready to enter
    Capturing,   // Pulling player in
    Completed    // Player fully absorbed
};

// Wave Portal - Goal that activates when all shards are collected
class WavePortal {
public:
    WavePortal(const glm::vec3& portalPos);
    ~WavePortal();
    
    void update(float dt);
    void render(unsigned int textureID);
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    float getRadius() const { return triggerRadius; }
    bool isActive() const { return state == PortalState::Active; }
    PortalState getState() const { return state; }
    glm::mat4 getModelMatrix() const;
    
    // Lifecycle control
    void spawn();  // Begin spawn animation (call when all shards collected)
    void activate();  // Legacy - now calls spawn()
    void startCapture();  // Begin pulling player in
    bool checkPlayerEntry(const glm::vec3& playerPos, float playerRadius);
    
    // Capture effect
    glm::vec3 pullPlayerToward(const glm::vec3& playerPos, float dt);  // Returns new player position
    
    void cleanup();
    void setupMesh();  // Initialize OpenGL buffers (can be called explicitly or lazy-loaded)
    
    // ON-RAILS: Public position for world scrolling
    glm::vec3 position;
    
private:
    PortalState state;  // Current lifecycle state
    
    float rotation;
    float rotationSpeed;
    float targetRotationSpeed;  // For smooth speed transitions
    float scale;
    float targetScale;          // For smooth scale transitions
    float triggerRadius;        // Collision radius (larger for big portal)
    
    bool active;  // Portal is open and ready (legacy - use state instead)
    bool playerInside;  // Player has entered
    
    // Spawn/activation animation
    float spawnAnimTimer;       // Timer for spawn animation (0 to 1)
    float spawnAnimDuration;    // How long spawn takes (e.g. 1.5 seconds)
    
    // Player capture (suck-in effect)
    float captureTimer;         // Timer for pull-in animation
    float captureDuration;      // How long capture takes (e.g. 1.5 seconds)
    float captureStrength;      // How strongly to pull player toward center
    
    // Activation animation and effects
    float activationTimer;
    float activationDuration;
    float activationBurstTimer;  // One-shot burst effect when activated
    float pulseTimer;
    float pulseSpeed;
    float lightIntensity;
    float targetLightIntensity;
    
    // Proximity glow (reacts when player is near)
    float proximityGlow;
    
    // Mesh (spinning ring)
    GLuint VAO, VBO, EBO;
    int indexCount;
    
    void loadPortalOBJ();    // Load Dr. Strange portal from OBJ
    void createRingMesh();   // Fallback simple ring
    void playActivationSound();  // Audio stub
};

#endif
