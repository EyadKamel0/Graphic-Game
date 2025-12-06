#ifndef SATELLITEFRAGMENT_H
#define SATELLITEFRAGMENT_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

/*
 * ═══════════════════════════════════════════════════════════════════════
 *  SATELLITE FRAGMENT - Level 2 Rotating Hazard
 * ═══════════════════════════════════════════════════════════════════════
 * 
 * Broken satellite pieces that spin slowly in space.
 * Uses satellite_obj.obj model from Release/Satellite folder.
 * Rotation makes them harder to dodge.
 */

// Submesh structure for multi-material OBJ models
struct SatelliteSubMesh {
    GLuint VAO, VBO, EBO;
    int indexCount;
    int materialIndex;
};

// Material structure for OBJ MTL files with PBR support
struct SatelliteMaterial {
    std::string name;
    glm::vec3 diffuseColor;
    GLuint diffuseTexture;      // BaseColor texture
    GLuint metallicTexture;     // Metallic texture
    GLuint roughnessTexture;    // Roughness texture
};

class SatelliteFragment {
public:
    SatelliteFragment(const glm::vec3& startPos, float rotationSpeed = 30.0f);
    ~SatelliteFragment();
    
    // Disable copy to prevent VAO/VBO/EBO handle duplication
    SatelliteFragment(const SatelliteFragment&) = delete;
    SatelliteFragment& operator=(const SatelliteFragment&) = delete;
    
    // Move constructor and assignment
    SatelliteFragment(SatelliteFragment&& other) noexcept;
    SatelliteFragment& operator=(SatelliteFragment&& other) noexcept;
    
    void update(float dt);
    void render(unsigned int textureID);
    void renderWithPBR();  // PBR rendering using loaded textures
    
    // Getters
    glm::vec3 getPosition() const { return position; }
    float getCollisionRadius() const { return collisionRadius; }
    glm::mat4 getModelMatrix() const;
    
    // Collision with player (uses bounding sphere)
    bool checkCollision(const glm::vec3& targetPos, float targetRadius) const;
    
    // Destruction state
    void destroy() { destroyed = true; }
    bool isDestroyed() const { return destroyed; }
    
    // Respawn at new position
    void respawn(const glm::vec3& newPos, float newRotSpeed = 30.0f);
    
    void cleanup();
    void setupMesh();
    
    // Static method to load shared mesh data
    static void loadSharedMesh();
    static void cleanupSharedMesh();
    static bool isSharedMeshLoaded() { return sharedMeshLoaded; }
    static const std::vector<SatelliteMaterial>& getMaterials() { return sharedMaterials; }
    static const std::vector<SatelliteSubMesh>& getSubMeshes() { return sharedSubMeshes; }
    
    // ON-RAILS: Public position for world scrolling
    glm::vec3 position;
    
private:
    glm::vec3 rotation;           // Current rotation angles (degrees)
    glm::vec3 rotationAxis;       // Axis to rotate around
    float rotationSpeed;          // Degrees per second
    float scale;
    float collisionRadius;        // Bounding sphere radius
    bool destroyed;               // Whether fragment was destroyed by collision
    
    // Static shared mesh data (loaded once, used by all instances)
    static std::vector<SatelliteSubMesh> sharedSubMeshes;
    static std::vector<SatelliteMaterial> sharedMaterials;
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
