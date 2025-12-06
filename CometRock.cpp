#include "CometRock.h"
#include "Texture.h"
#include <cmath>
#include <iostream>
#include <random>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>

// Static member initialization
std::vector<CometRock::Submesh> CometRock::sharedSubmeshes;
std::vector<CometRock::Material> CometRock::sharedMaterials;
bool CometRock::modelLoaded = false;

CometRock::CometRock(const glm::vec3& startPos, const glm::vec3& vel, float rad)
    : position(startPos),
      velocity(vel),
      previousPosition(startPos),
      rotation(0.0f, 0.0f, 0.0f),
      radius(rad),
      collisionRadius(1.5f),  // Tight hitbox matching visual size
      scale(1.2f),            // Double size for visibility
      initialScale(1.2f),
      state(State::Normal),
      breakTimer(0.0f),
      breakDuration(0.2f) {
    
    // Random rotation for variety
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> rotSpeedDist(-120.0f, 120.0f);
    
    rotation = glm::vec3(rotDist(gen), rotDist(gen), rotDist(gen));
    rotationSpeed = glm::vec3(rotSpeedDist(gen), rotSpeedDist(gen), rotSpeedDist(gen));
    
    // Load shared model if not already loaded
    if (!modelLoaded) {
        loadModel();
    }
}

CometRock::~CometRock() {
    cleanup();
}

// Move constructor
CometRock::CometRock(CometRock&& other) noexcept
    : position(other.position),
      velocity(other.velocity),
      previousPosition(other.previousPosition),
      rotation(other.rotation),
      rotationSpeed(other.rotationSpeed),
      radius(other.radius),
      collisionRadius(other.collisionRadius),
      scale(other.scale),
      initialScale(other.initialScale),
      state(other.state),
      breakTimer(other.breakTimer),
      breakDuration(other.breakDuration) {
}

// Move assignment operator
CometRock& CometRock::operator=(CometRock&& other) noexcept {
    if (this != &other) {
        position = other.position;
        velocity = other.velocity;
        previousPosition = other.previousPosition;
        rotation = other.rotation;
        rotationSpeed = other.rotationSpeed;
        radius = other.radius;
        collisionRadius = other.collisionRadius;
        scale = other.scale;
        initialScale = other.initialScale;
        state = other.state;
        breakTimer = other.breakTimer;
        breakDuration = other.breakDuration;
    }
    return *this;
}

void CometRock::loadModel() {
    if (modelLoaded) return;
    
    const std::string basePath = "Level2_Small/";
    const std::string objPath = basePath + "Asteroid_1d.obj";
    const std::string mtlPath = basePath + "Asteroid_1d.mtl";
    
    std::cout << "[COMET] Loading model from: " << objPath << std::endl;
    
    // Load the color texture directly
    std::string colorTexPath = basePath + "Asteroid1d_Color_2K.png";
    std::ifstream colorTest(colorTexPath);
    if (colorTest.good()) {
        colorTest.close();
        Material mat;
        mat.name = "Asteroid1d_Crystal";
        mat.baseColorTexture = loadTexture(colorTexPath.c_str());
        sharedMaterials.push_back(mat);
        std::cout << "[COMET] Loaded color texture: " << colorTexPath << std::endl;
    }
    
    // Parse OBJ file
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;
    
    std::map<std::string, std::vector<float>> materialVertices;
    std::map<std::string, std::vector<unsigned int>> materialIndices;
    std::map<std::string, std::map<std::string, unsigned int>> materialVertexMap;
    
    std::string currentMaterial = "default";
    
    std::ifstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "[COMET] Failed to load OBJ: " << objPath << std::endl;
        modelLoaded = true;
        return;
    }
    
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
        else if (prefix == "usemtl") {
            iss >> currentMaterial;
        }
        else if (prefix == "f") {
            std::vector<std::string> faceVerts;
            std::string vert;
            while (iss >> vert) {
                faceVerts.push_back(vert);
            }
            
            if (faceVerts.size() < 3) continue;
            
            auto processFaceVertex = [&](const std::string& faceStr) -> unsigned int {
                auto& vertMap = materialVertexMap[currentMaterial];
                auto it = vertMap.find(faceStr);
                if (it != vertMap.end()) {
                    return it->second;
                }
                
                unsigned int vIdx = 0, vtIdx = 0, vnIdx = 0;
                
                if (sscanf(faceStr.c_str(), "%u/%u/%u", &vIdx, &vtIdx, &vnIdx) != 3) {
                    if (sscanf(faceStr.c_str(), "%u//%u", &vIdx, &vnIdx) != 2) {
                        if (sscanf(faceStr.c_str(), "%u/%u", &vIdx, &vtIdx) != 2) {
                            sscanf(faceStr.c_str(), "%u", &vIdx);
                        }
                    }
                }
                
                auto& vertices = materialVertices[currentMaterial];
                unsigned int index = vertices.size() / 8;
                
                if (vIdx > 0 && vIdx <= temp_vertices.size()) {
                    vertices.push_back(temp_vertices[vIdx - 1].x);
                    vertices.push_back(temp_vertices[vIdx - 1].y);
                    vertices.push_back(temp_vertices[vIdx - 1].z);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
                
                if (vnIdx > 0 && vnIdx <= temp_normals.size()) {
                    vertices.push_back(temp_normals[vnIdx - 1].x);
                    vertices.push_back(temp_normals[vnIdx - 1].y);
                    vertices.push_back(temp_normals[vnIdx - 1].z);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(1.0f);
                    vertices.push_back(0.0f);
                }
                
                if (vtIdx > 0 && vtIdx <= temp_uvs.size()) {
                    vertices.push_back(temp_uvs[vtIdx - 1].x);
                    vertices.push_back(temp_uvs[vtIdx - 1].y);
                } else {
                    vertices.push_back(0.0f);
                    vertices.push_back(0.0f);
                }
                
                vertMap[faceStr] = index;
                return index;
            };
            
            // Triangulate face
            auto& indices = materialIndices[currentMaterial];
            unsigned int first = processFaceVertex(faceVerts[0]);
            for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
                indices.push_back(first);
                indices.push_back(processFaceVertex(faceVerts[i]));
                indices.push_back(processFaceVertex(faceVerts[i + 1]));
            }
        }
    }
    objFile.close();
    
    // Create submeshes for each material
    for (auto& pair : materialVertices) {
        const std::string& matName = pair.first;
        auto& vertices = pair.second;
        auto& indices = materialIndices[matName];
        
        if (vertices.empty() || indices.empty()) continue;
        
        Submesh submesh;
        submesh.materialName = matName;
        submesh.indexCount = indices.size();
        
        glGenVertexArrays(1, &submesh.VAO);
        glGenBuffers(1, &submesh.VBO);
        glGenBuffers(1, &submesh.EBO);
        
        glBindVertexArray(submesh.VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, submesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, submesh.EBO);
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
        
        sharedSubmeshes.push_back(submesh);
        std::cout << "[COMET] Created submesh: " << matName << " with " << indices.size() / 3 << " triangles" << std::endl;
    }
    
    modelLoaded = true;
    std::cout << "[COMET] Model loaded with " << sharedSubmeshes.size() << " submeshes and " 
              << sharedMaterials.size() << " materials" << std::endl;
}

void CometRock::cleanupSharedResources() {
    for (auto& submesh : sharedSubmeshes) {
        if (submesh.VAO != 0) glDeleteVertexArrays(1, &submesh.VAO);
        if (submesh.VBO != 0) glDeleteBuffers(1, &submesh.VBO);
        if (submesh.EBO != 0) glDeleteBuffers(1, &submesh.EBO);
    }
    sharedSubmeshes.clear();
    
    for (auto& mat : sharedMaterials) {
        if (mat.baseColorTexture != 0) glDeleteTextures(1, &mat.baseColorTexture);
    }
    sharedMaterials.clear();
    
    modelLoaded = false;
}

void CometRock::update(float dt) {
    rotation += rotationSpeed * dt;
    
    switch (state) {
        case State::Normal:
            previousPosition = position;  // Store for swept collision
            position += velocity * dt;
            break;
            
        case State::Breaking:
            breakTimer += dt;
            if (breakTimer >= breakDuration) {
                state = State::Dead;
            } else {
                float progress = breakTimer / breakDuration;
                scale = initialScale * (1.0f - progress);
            }
            break;
            
        case State::Dead:
            break;
    }
}

bool CometRock::checkCollision(const glm::vec3& bulletPos, float bulletRadius) const {
    if (state != State::Normal) return false;
    
    float distance = glm::length(position - bulletPos);
    return distance < (collisionRadius + bulletRadius);
}

bool CometRock::checkPlayerCollision(const glm::vec3& playerPos, float playerRadius) const {
    if (state != State::Normal) return false;
    
    // Swept collision detection - check along the path the comet traveled
    glm::vec3 movement = position - previousPosition;
    float movementLen = glm::length(movement);
    
    if (movementLen < 0.01f) {
        // Barely moved, just check current position
        float distance = glm::length(position - playerPos);
        return distance < (collisionRadius + playerRadius);
    }
    
    // Check multiple points along the movement path
    int steps = std::max(1, static_cast<int>(movementLen / collisionRadius) + 1);
    for (int i = 0; i <= steps; i++) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        glm::vec3 checkPos = previousPosition + movement * t;
        float distance = glm::length(checkPos - playerPos);
        if (distance < (collisionRadius + playerRadius)) {
            return true;
        }
    }
    
    return false;
}

void CometRock::startBreaking() {
    if (state != State::Normal) return;
    
    state = State::Breaking;
    breakTimer = 0.0f;
    initialScale = scale;
    
    std::cout << "[COMET] Comet destroyed!" << std::endl;
}

void CometRock::respawn(const glm::vec3& newPos, const glm::vec3& newVel, float newRadius) {
    position = newPos;
    previousPosition = newPos;  // Reset for swept collision
    velocity = newVel;
    radius = newRadius;
    scale = 1.2f;
    initialScale = 1.2f;
    state = State::Normal;
    breakTimer = 0.0f;
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> rotSpeedDist(-120.0f, 120.0f);
    
    rotation = glm::vec3(rotDist(gen), rotDist(gen), rotDist(gen));
    rotationSpeed = glm::vec3(rotSpeedDist(gen), rotSpeedDist(gen), rotSpeedDist(gen));
}

glm::mat4 CometRock::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    model = glm::scale(model, glm::vec3(scale));
    
    return model;
}

void CometRock::render(unsigned int fallbackTexture) {
    if (state == State::Dead) return;
    if (sharedSubmeshes.empty()) return;
    
    for (auto& submesh : sharedSubmeshes) {
        // Find matching material
        GLuint textureToUse = fallbackTexture;
        for (auto& mat : sharedMaterials) {
            if (mat.name == submesh.materialName || 
                submesh.materialName.find(mat.name) != std::string::npos ||
                mat.name.find(submesh.materialName) != std::string::npos) {
                if (mat.baseColorTexture != 0) {
                    textureToUse = mat.baseColorTexture;
                    break;
                }
            }
        }
        
        // Try matching by index
        if (textureToUse == fallbackTexture && !sharedMaterials.empty()) {
            for (size_t i = 0; i < sharedSubmeshes.size(); ++i) {
                if (&sharedSubmeshes[i] == &submesh && i < sharedMaterials.size()) {
                    if (sharedMaterials[i].baseColorTexture != 0) {
                        textureToUse = sharedMaterials[i].baseColorTexture;
                    }
                    break;
                }
            }
        }
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureToUse);
        
        glBindVertexArray(submesh.VAO);
        glDrawElements(GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

void CometRock::cleanup() {
    // Individual instances don't own the shared resources
}
