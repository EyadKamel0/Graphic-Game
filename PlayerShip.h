#ifndef PLAYERSHIP_H
#define PLAYERSHIP_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <map>

struct Material {
    std::string name;
    GLuint diffuseTexture;
    glm::vec3 diffuseColor;
    bool usesBakedLighting;
};

struct SubMesh {
    GLuint VAO, VBO, EBO;
    int indexCount;
    int materialIndex;
};

/*
 * ON-RAILS ARCADE SHOOTER - FAKE 3D SYSTEM
 * 
 * The player ship moves in a 2D plane (X/Y) within a 3D world.
 * The Z position is fixed - the ship stays at a constant distance from the camera.
 * Movement is restricted to left/right/up/down on screen.
 * Bullets always shoot straight forward along -Z direction.
 * 
 * Controls:
 *   W: Move ship UP (positive Y)
 *   S: Move ship DOWN (negative Y)
 *   A: Move ship LEFT (negative X)
 *   D: Move ship RIGHT (positive X)
 *   Space: Booster (future feature)
 *   Left Mouse: Shoot straight ahead
 *   Right Mouse: Toggle camera (First/Third person)
 *   Mouse Movement: NO EFFECT (disabled)
 */

class PlayerShip {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    
    // Movement parameters (2D only - X and Y)
    float moveSpeed;        // Lateral movement speed (left/right/up/down)
    float moveDamping;      // How quickly ship slows down when no input
    float moveAccel;        // How quickly ship accelerates to target velocity (smoothing)
    
    // Movement bounds (prevent ship from going off-screen)
    float boundMinX, boundMaxX;
    float boundMinY, boundMaxY;
    float fixedZ;           // Fixed Z position (on-rails)
    
    // Visual banking (tilt left/right when moving, purely cosmetic)
    float bankAngle;        // Current visual roll angle
    float maxBankAngle;     // Maximum visual tilt
    float bankSpeed;        // How fast ship tilts
    
    std::vector<SubMesh> subMeshes;
    std::vector<Material> materials;
    
    // Power-up state
    bool jetBoosterActive;
    
    // Jet Booster burst system
    int jetBoosterCharges;        // Number of boost charges available
    bool jetBoosterBurstActive;   // Currently in a boost burst
    float jetBoosterBurstTimer;   // Time remaining in current burst
    static constexpr float JET_BOOSTER_BURST_DURATION = 2.0f;
    bool spaceWasPressed;         // For edge detection
    
    // ═══════════════════════════════════════════════════════════
    // HEALTH SYSTEM - Level 1 Combat (3 hits → game over)
    // ═══════════════════════════════════════════════════════════
    int maxHealth;          // Ship has 3 health; on 0, Level 1 enters Game Over state
    int currentHealth;
    bool isAlive;
    
    // Hit feedback: flashing + knockback + short invulnerability
    bool isInvulnerable;    // Temporary invulnerability after taking damage
    float invulnerableTimer;
    const float INVULNERABLE_DURATION = 2.0f;  // 2 seconds of invulnerability
    
    bool isHitFlashing;     // Hit flash indicates damage just taken
    float hitFlashTimer;
    const float HIT_FLASH_DURATION = 0.6f;  // 0.6 seconds of visual flash
    
    // Engine flame rendering
    GLuint flameVAO, flameVBO;
    float flameAnimTimer;    // Animation timer for flame flicker
    GLuint flameShaderProgram;  // Simple shader for flame rendering
    
    PlayerShip();
    ~PlayerShip();
    
    void update(float deltaTime);
    void render(class Shader& shader);
    void renderEngineFlames(const glm::mat4& view, const glm::mat4& projection);  // Render engine exhaust flames
    void processInput(GLFWwindow* window, float deltaTime);
    
    glm::mat4 getModelMatrix() const;
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getForward() const { return glm::vec3(0.0f, 0.0f, -1.0f); }  // Always straight ahead
    
    // Setter for collision response
    void setPosition(const glm::vec3& newPos);
    
    // Power-up control
    void setJetBoosterActive(bool active) { jetBoosterActive = active; }
    void addJetBoosterCharge() { jetBoosterCharges++; }
    int getJetBoosterCharges() const { return jetBoosterCharges; }
    bool isJetBoosterBurstActive() const { return jetBoosterBurstActive; }
    
    // Health & damage system
    void takeDamage(int amount = 1);  // Apply damage (triggers invulnerability + flash)
    void heal(int amount);             // Restore health
    void resetHealth();                // Full heal (for level restart)
    int getHealth() const { return currentHealth; }
    int getMaxHealth() const { return maxHealth; }
    bool getIsAlive() const { return isAlive; }
    bool getIsInvulnerable() const { return isInvulnerable; }
    bool getIsHitFlashing() const { return isHitFlashing; }
    
private:
    void setupMesh();
    void clampPositionToBounds();
};

#endif
