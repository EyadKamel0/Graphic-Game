#include "Level2Collectible.h"
#include "Texture.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <vector>

const float PI = 3.14159265359f;

// Static member initialization
Level2CollectibleSubMesh Level2Collectible::sharedMesh_1_1;
Level2CollectibleSubMesh Level2Collectible::sharedMesh_1_3;
bool Level2Collectible::sharedMeshesLoaded = false;

Level2Collectible::Level2Collectible(const glm::vec3& startPos, Level2CollectibleType t)
    : position(startPos),
      rotation(0.0f, 0.0f, 0.0f),
      type(t),
      radius(1.5f),  // Larger pickup radius for easier collection
      scale(0.02f),  // Smaller scale for collectibles
      collected(false),
      bobTimer(0.0f),
      bobSpeed(2.0f),
      bobAmount(0.5f),
      originalY(startPos.y),
      glowIntensity(2.0f),
      glowPulseTimer(0.0f),
      glowPulseSpeed(3.0f),
      glowMin(1.5f),
      glowMax(3.0f),
      pickupAnimTimer(0.0f),
      pickupAnimDuration(0.3f),
      VAO(0), VBO(0), EBO(0),
      indexCount(0),
      usesSharedMesh(false) {
    
    // Adjust properties based on type
    switch (type) {
        case Level2CollectibleType::RedEnergyCell:
            rotationSpeed = glm::vec3(30.0f, 60.0f, 40.0f);
            bobSpeed = 2.5f;
            glowPulseSpeed = 4.0f;
            glowMin = 1.5f;
            glowMax = 3.5f;
            break;
            
        case Level2CollectibleType::RapidFire:
        case Level2CollectibleType::Invincibility:
            rotationSpeed = glm::vec3(40.0f, 80.0f, 50.0f);
            bobSpeed = 3.0f;
            bobAmount = 0.6f;
            glowPulseSpeed = 5.0f;
            glowMin = 2.0f;
            glowMax = 4.0f;
            break;
    }
    
    // Random initial phase
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * PI);
    bobTimer = phaseDist(gen);
    glowPulseTimer = phaseDist(gen);
}

Level2Collectible::~Level2Collectible() {
    cleanup();
}

// Move constructor
Level2Collectible::Level2Collectible(Level2Collectible&& other) noexcept
    : position(other.position),
      rotation(other.rotation),
      rotationSpeed(other.rotationSpeed),
      type(other.type),
      radius(other.radius),
      scale(other.scale),
      collected(other.collected),
      bobTimer(other.bobTimer),
      bobSpeed(other.bobSpeed),
      bobAmount(other.bobAmount),
      originalY(other.originalY),
      glowIntensity(other.glowIntensity),
      glowPulseTimer(other.glowPulseTimer),
      glowPulseSpeed(other.glowPulseSpeed),
      glowMin(other.glowMin),
      glowMax(other.glowMax),
      pickupAnimTimer(other.pickupAnimTimer),
      pickupAnimDuration(other.pickupAnimDuration),
      VAO(other.VAO),
      VBO(other.VBO),
      EBO(other.EBO),
      indexCount(other.indexCount),
      usesSharedMesh(other.usesSharedMesh) {
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.indexCount = 0;
}

// Move assignment operator
Level2Collectible& Level2Collectible::operator=(Level2Collectible&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        position = other.position;
        rotation = other.rotation;
        rotationSpeed = other.rotationSpeed;
        type = other.type;
        radius = other.radius;
        scale = other.scale;
        collected = other.collected;
        bobTimer = other.bobTimer;
        bobSpeed = other.bobSpeed;
        bobAmount = other.bobAmount;
        originalY = other.originalY;
        glowIntensity = other.glowIntensity;
        glowPulseTimer = other.glowPulseTimer;
        glowPulseSpeed = other.glowPulseSpeed;
        glowMin = other.glowMin;
        glowMax = other.glowMax;
        pickupAnimTimer = other.pickupAnimTimer;
        pickupAnimDuration = other.pickupAnimDuration;
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        indexCount = other.indexCount;
        usesSharedMesh = other.usesSharedMesh;
        
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.indexCount = 0;
    }
    return *this;
}

void Level2Collectible::loadSharedMeshes() {
    if (sharedMeshesLoaded) return;
    
    std::cout << "[L2 COLLECTIBLE] Loading shared meshes..." << std::endl;
    
    const std::string basePath = "Collectibles/";
    const std::string objPath = basePath + "Cylinder_Sci_Fi_1.obj";
    
    // Load 1_1 texture for red energy cells (Level 2 collectibles)
    loadOBJWithTexture(objPath, basePath + "TX_Cylinder_Sci_Fi_1_1_Base_color.png", sharedMesh_1_1);
    
    // Load 1_3 texture for power-ups
    loadOBJWithTexture(objPath, basePath + "TX_Cylinder_Sci_Fi_1_3_Base_color.png", sharedMesh_1_3);
    
    if (sharedMesh_1_1.VAO != 0 && sharedMesh_1_3.VAO != 0) {
        sharedMeshesLoaded = true;
        std::cout << "[L2 COLLECTIBLE] Shared meshes loaded successfully" << std::endl;
    } else {
        std::cerr << "[L2 COLLECTIBLE] Failed to load some shared meshes" << std::endl;
    }
}

void Level2Collectible::cleanupSharedMeshes() {
    auto cleanupMesh = [](Level2CollectibleSubMesh& mesh) {
        if (mesh.VAO != 0) glDeleteVertexArrays(1, &mesh.VAO);
        if (mesh.VBO != 0) glDeleteBuffers(1, &mesh.VBO);
        if (mesh.EBO != 0) glDeleteBuffers(1, &mesh.EBO);
        if (mesh.texture != 0) glDeleteTextures(1, &mesh.texture);
        mesh.VAO = mesh.VBO = mesh.EBO = mesh.texture = 0;
        mesh.indexCount = 0;
    };
    
    cleanupMesh(sharedMesh_1_1);
    cleanupMesh(sharedMesh_1_3);
    sharedMeshesLoaded = false;
}

void Level2Collectible::loadOBJWithTexture(const std::string& objPath, const std::string& texturePath, Level2CollectibleSubMesh& outMesh) {
    std::ifstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "[L2 COLLECTIBLE] Failed to open OBJ: " << objPath << std::endl;
        return;
    }
    
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    std::string line;
    while (std::getline(objFile, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        }
        else if (prefix == "vt") {
            glm::vec2 uv;
            iss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (prefix == "vn") {
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        }
        else if (prefix == "f") {
            std::vector<std::string> faceVerts;
            std::string vert;
            while (iss >> vert) {
                faceVerts.push_back(vert);
            }
            
            if (faceVerts.size() < 3) continue;
            
            auto processFaceVertex = [&](const std::string& faceStr) -> unsigned int {
                unsigned int vIdx = 0, vtIdx = 0, vnIdx = 0;
                
                if (sscanf(faceStr.c_str(), "%u/%u/%u", &vIdx, &vtIdx, &vnIdx) != 3) {
                    if (sscanf(faceStr.c_str(), "%u//%u", &vIdx, &vnIdx) != 2) {
                        if (sscanf(faceStr.c_str(), "%u/%u", &vIdx, &vtIdx) != 2) {
                            sscanf(faceStr.c_str(), "%u", &vIdx);
                        }
                    }
                }
                
                unsigned int index = vertices.size() / 8;
                
                // pos(3) + normal(3) + uv(2)
                if (vIdx > 0 && vIdx <= temp_vertices.size()) {
                    vertices.push_back(temp_vertices[vIdx-1].x);
                    vertices.push_back(temp_vertices[vIdx-1].y);
                    vertices.push_back(temp_vertices[vIdx-1].z);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
                if (vnIdx > 0 && vnIdx <= temp_normals.size()) {
                    vertices.push_back(temp_normals[vnIdx-1].x);
                    vertices.push_back(temp_normals[vnIdx-1].y);
                    vertices.push_back(temp_normals[vnIdx-1].z);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                }
                if (vtIdx > 0 && vtIdx <= temp_uvs.size()) {
                    vertices.push_back(temp_uvs[vtIdx-1].x);
                    vertices.push_back(temp_uvs[vtIdx-1].y);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
                
                return index;
            };
            
            // Triangulate face
            unsigned int firstIdx = processFaceVertex(faceVerts[0]);
            unsigned int prevIdx = processFaceVertex(faceVerts[1]);
            
            for (size_t i = 2; i < faceVerts.size(); i++) {
                unsigned int currIdx = processFaceVertex(faceVerts[i]);
                indices.push_back(firstIdx);
                indices.push_back(prevIdx);
                indices.push_back(currIdx);
                prevIdx = currIdx;
            }
        }
    }
    objFile.close();
    
    if (vertices.empty() || indices.empty()) {
        std::cerr << "[L2 COLLECTIBLE] No geometry loaded from " << objPath << std::endl;
        return;
    }
    
    std::cout << "[L2 COLLECTIBLE] Loaded " << temp_vertices.size() << " vertices, " 
              << indices.size() / 3 << " triangles" << std::endl;
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &outMesh.VAO);
    glGenBuffers(1, &outMesh.VBO);
    glGenBuffers(1, &outMesh.EBO);
    
    glBindVertexArray(outMesh.VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, outMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
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
    
    outMesh.indexCount = indices.size();
    
    // Load texture
    outMesh.texture = loadTexture(texturePath);
    if (outMesh.texture != 0) {
        std::cout << "[L2 COLLECTIBLE] Loaded texture: " << texturePath << std::endl;
    } else {
        std::cerr << "[L2 COLLECTIBLE] Failed to load texture: " << texturePath << std::endl;
    }
}

void Level2Collectible::setupMesh() {
    if (sharedMeshesLoaded) {
        usesSharedMesh = true;
        return;
    }
    
    // Fallback to simple mesh if shared mesh not loaded
    if (VAO == 0) {
        createFallbackMesh();
    }
}

void Level2Collectible::createFallbackMesh() {
    // Create hexagonal prism as fallback
    const int sides = 6;
    float hexRadius = 0.5f;
    float height = 0.8f;
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Generate hexagonal vertices (simplified)
    for (int i = 0; i < sides; i++) {
        float angle = (float)i / (float)sides * 2.0f * PI;
        float x = std::cos(angle) * hexRadius;
        float z = std::sin(angle) * hexRadius;
        
        // Top vertex
        vertices.push_back(x);
        vertices.push_back(height * 0.5f);
        vertices.push_back(z);
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(z);  // normal
        vertices.push_back((float)i / sides); vertices.push_back(1.0f);  // uv
        
        // Bottom vertex
        vertices.push_back(x);
        vertices.push_back(-height * 0.5f);
        vertices.push_back(z);
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(z);
        vertices.push_back((float)i / sides); vertices.push_back(0.0f);
    }
    
    // Side faces
    for (int i = 0; i < sides; i++) {
        int next = (i + 1) % sides;
        unsigned int topCurr = i * 2;
        unsigned int botCurr = i * 2 + 1;
        unsigned int topNext = next * 2;
        unsigned int botNext = next * 2 + 1;
        
        indices.push_back(topCurr);
        indices.push_back(botCurr);
        indices.push_back(topNext);
        
        indices.push_back(topNext);
        indices.push_back(botCurr);
        indices.push_back(botNext);
    }
    
    indexCount = indices.size();
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void Level2Collectible::update(float dt) {
    rotation.x += rotationSpeed.x * dt;
    rotation.y += rotationSpeed.y * dt;
    rotation.z += rotationSpeed.z * dt;
    
    if (!collected) {
        bobTimer += bobSpeed * dt;
        position.y = originalY + sin(bobTimer) * bobAmount;
        
        // Pulse glow
        glowPulseTimer += glowPulseSpeed * dt;
        glowIntensity = glowMin + (glowMax - glowMin) * (0.5f + 0.5f * sin(glowPulseTimer));
    } else {
        pickupAnimTimer += dt;
        float progress = pickupAnimTimer / pickupAnimDuration;
        if (progress > 1.0f) progress = 1.0f;
        scale = 0.02f * (1.0f - progress);
    }
}

bool Level2Collectible::checkCollision(const glm::vec3& playerPos, float playerRadius) {
    if (collected) return false;
    float distance = glm::length(position - playerPos);
    return distance < (radius + playerRadius);
}

void Level2Collectible::collect() {
    if (collected) return;
    collected = true;
    pickupAnimTimer = 0.0f;
    
    switch (type) {
        case Level2CollectibleType::RedEnergyCell:
            std::cout << "[COLLECT] Red Energy Cell collected!" << std::endl;
            break;
        case Level2CollectibleType::RapidFire:
            std::cout << "[COLLECT] Rapid Fire power-up activated!" << std::endl;
            break;
        case Level2CollectibleType::Invincibility:
            std::cout << "[COLLECT] Invincibility Shield activated!" << std::endl;
            break;
    }
    playPickupSound();
}

void Level2Collectible::respawn(const glm::vec3& newPos) {
    position = newPos;
    collected = false;
    pickupAnimTimer = 0.0f;
    bobTimer = 0.0f;
    glowPulseTimer = 0.0f;
}

void Level2Collectible::playPickupSound() {
    // TODO: Implement audio system
}

glm::vec3 Level2Collectible::getGlowColor() const {
    switch (type) {
        case Level2CollectibleType::RedEnergyCell:
            return glm::vec3(1.0f, 0.2f, 0.1f);  // Bright red
        case Level2CollectibleType::RapidFire:
            return glm::vec3(1.0f, 0.6f, 0.0f);  // Orange for rapid fire
        case Level2CollectibleType::Invincibility:
            return glm::vec3(0.2f, 0.8f, 1.0f);  // Cyan for invincibility
        default:
            return glm::vec3(1.0f, 1.0f, 1.0f);
    }
}

glm::mat4 Level2Collectible::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(scale));
    return model;
}

void Level2Collectible::render(unsigned int textureID) {
    if (collected) return;
    
    if (usesSharedMesh && sharedMeshesLoaded) {
        // Select appropriate shared mesh based on type
        Level2CollectibleSubMesh* mesh = nullptr;
        if (type == Level2CollectibleType::RedEnergyCell) {
            mesh = &sharedMesh_1_1;  // 1_1 for red energy cells
        } else {
            mesh = &sharedMesh_1_3;  // 1_3 for power-ups
        }
        
        if (mesh && mesh->VAO != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh->texture);
            glBindVertexArray(mesh->VAO);
            glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    } else {
        // Fallback rendering
        if (VAO == 0 || indexCount == 0) return;
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

void Level2Collectible::cleanup() {
    if (!usesSharedMesh) {
        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            VAO = 0;
        }
        if (VBO != 0) {
            glDeleteBuffers(1, &VBO);
            VBO = 0;
        }
        if (EBO != 0) {
            glDeleteBuffers(1, &EBO);
            EBO = 0;
        }
    }
}
