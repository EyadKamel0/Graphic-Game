#ifndef SWINGSOLARPANEL_H
#define SWINGSOLARPANEL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

/*
 * ═══════════════════════════════════════════════════════════════════════
 *  SWINGING SOLAR PANEL - Level 2 Pendulum Hazard
 * ═══════════════════════════════════════════════════════════════════════
 * 
 * Large solar panels swinging like pendulums left-to-right.
 * Time-based obstacle requiring careful movement.
 * Uses OBJ model from SolarPanel folder.
 */

// Submesh structure for multi-material OBJ models
struct SolarPanelSubMesh {
    GLuint VAO, VBO, EBO;
    int indexCount;
    int materialIndex;
};

// Material structure for OBJ MTL files with PBR support
struct SolarPanelMaterial {
    std::string name;
    glm::vec3 diffuseColor;
    GLuint diffuseTexture;      // map_Kd - albedo/diffuse
    GLuint metallicTexture;     // map_refl - metallic
    GLuint roughnessTexture;    // map_Ns - roughness
};

class SwingSolarPanel {
public:
    SwingSolarPanel(const glm::vec3& pivotPos, float swingAngle = 45.0f, float swingSpeed = 1.0f);
    ~SwingSolarPanel();
    
    // Disable copy to prevent VAO/VBO/EBO handle duplication
    SwingSolarPanel(const SwingSolarPanel&) = delete;
    SwingSolarPanel& operator=(const SwingSolarPanel&) = delete;
    
    // Move constructor and assignment
    SwingSolarPanel(SwingSolarPanel&& other) noexcept;
    SwingSolarPanel& operator=(SwingSolarPanel&& other) noexcept;
    
    void update(float dt);
    void render(unsigned int textureID);
    void renderWithPBR();  // PBR rendering using loaded textures
    
    // Getters
    glm::vec3 getPivotPosition() const { return pivotPosition; }
    glm::vec3 getPanelCenterPosition() const;  // Actual center of the panel
    float getCollisionRadius() const { return collisionRadius; }
    glm::mat4 getModelMatrix() const;
    
    // Collision with player (uses bounding box approximation via sphere)
    bool checkCollision(const glm::vec3& targetPos, float targetRadius) const;
    
    // Destruction state
    void destroy() { destroyed = true; }
    bool isDestroyed() const { return destroyed; }
    
    // Respawn at new position
    void respawn(const glm::vec3& newPivotPos, float newSwingAngle = 45.0f, float newSwingSpeed = 1.0f);
    
    void cleanup();
    void setupMesh();
    
    // Static method to load shared mesh data
    static void loadSharedMesh();
    static void cleanupSharedMesh();
    static bool isSharedMeshLoaded() { return sharedMeshLoaded; }
    static const std::vector<SolarPanelMaterial>& getMaterials();
    static const std::vector<SolarPanelSubMesh>& getSubMeshes();
    
    // ON-RAILS: Public pivot position for world scrolling
    glm::vec3 pivotPosition;
    
private:
    // Pendulum motion parameters
    float swingAngle;         // Maximum swing angle (degrees from vertical)
    float swingSpeed;         // Oscillation speed (radians per second multiplier)
    float currentSwingAngle;  // Current angle in the swing
    float swingTimer;         // Timer for pendulum motion
    float panelLength;        // Distance from pivot to panel center
    
    float scale;
    float collisionRadius;    // Bounding sphere radius for panel
    bool destroyed;           // Whether panel was destroyed by collision
    
    // Static shared mesh data (loaded once, used by all instances)
    static std::vector<SolarPanelSubMesh> sharedSubMeshes;
    static std::vector<SolarPanelMaterial> sharedMaterials;
    static bool sharedMeshLoaded;
    
    // Instance uses shared mesh
    bool usesSharedMesh;
    
    // Fallback mesh (if OBJ loading fails)
    GLuint VAO, VBO, EBO;
    int indexCount;
    
    void createFallbackMesh();
    static void loadOBJModel(const std::string& objPath, const std::string& mtlPath);
};

#endif
