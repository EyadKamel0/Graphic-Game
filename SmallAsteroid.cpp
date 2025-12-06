#include "SmallAsteroid.h"
#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

// ============================================================================
// SmallAsteroid Implementation
// ============================================================================

SmallAsteroid::SmallAsteroid(const glm::vec3& startPos, const glm::vec3& vel, float rad)
    : position(startPos), velocity(vel), radius(rad), scale(1.0f), initialScale(1.0f),
      state(State::Normal), breakTimer(0.0f), breakDuration(0.25f),
      VAO(0), VBO(0), EBO(0), indexCount(0) {
    
    // Random rotation for visual variety
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> rotSpeedDist(-60.0f, 60.0f);
    
    rotation = glm::vec3(rotDist(gen), rotDist(gen), rotDist(gen));
    rotationSpeed = glm::vec3(rotSpeedDist(gen), rotSpeedDist(gen), rotSpeedDist(gen));
    
    // Setup mesh immediately to avoid lazy loading during render
    setupMesh();
}

SmallAsteroid::~SmallAsteroid() {
    cleanup();
}

void SmallAsteroid::setupMesh() {
    if (VAO != 0) return;  // Already set up
    
    // Load the A2.obj model
    const std::string objPath = "Breakabel_Asteroids/A2.obj";
    
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    std::ifstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "Failed to load small asteroid OBJ: " << objPath << std::endl;
        // Fallback to sphere
        createSphereMesh(12);
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
                
                // Add vertex data: pos(3) + normal(3) + uv(2)
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
    
    indexCount = indices.size();
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
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
}

void SmallAsteroid::createSphereMesh(int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Generate sphere vertices (same as Asteroid.cpp)
    for (int lat = 0; lat <= segments; ++lat) {
        float theta = lat * glm::pi<float>() / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= segments; ++lon) {
            float phi = lon * 2.0f * glm::pi<float>() / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            float u = (float)lon / segments;
            float v = (float)lat / segments;
            
            vertices.insert(vertices.end(), {
                x, y, z,        // Position
                x, y, z,        // Normal
                u, v            // TexCoord
            });
        }
    }
    
    // Generate indices
    for (int lat = 0; lat < segments; ++lat) {
        for (int lon = 0; lon < segments; ++lon) {
            int first = lat * (segments + 1) + lon;
            int second = first + segments + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
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

void SmallAsteroid::update(float dt) {
    if (state == State::Normal) {
        // Normal movement
        position += velocity * dt;
        rotation += rotationSpeed * dt;
        
        // Wrap rotation
        rotation.x = fmod(rotation.x, 360.0f);
        rotation.y = fmod(rotation.y, 360.0f);
        rotation.z = fmod(rotation.z, 360.0f);
    }
    else if (state == State::Breaking) {
        // Breaking animation
        breakTimer += dt;
        
        // Spin faster during break
        rotation += rotationSpeed * 3.0f * dt;
        
        // Scale animation: expand briefly then shrink quickly
        float progress = breakTimer / breakDuration;
        if (progress < 0.3f) {
            // Expand phase
            scale = initialScale * (1.0f + progress * 1.5f);
        } else {
            // Shrink phase
            float shrinkProgress = (progress - 0.3f) / 0.7f;
            scale = initialScale * (1.45f - shrinkProgress * 1.5f);
        }
        
        if (breakTimer >= breakDuration) {
            state = State::Dead;
        }
    }
}

glm::mat4 SmallAsteroid::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(radius * scale));
    return model;
}

void SmallAsteroid::render(unsigned int textureID) {
    if (state == State::Dead) return;
    
    // Lazy initialization
    if (VAO == 0) {
        setupMesh();
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool SmallAsteroid::checkCollision(const glm::vec3& bulletPos, float bulletRadius) const {
    if (state != State::Normal) return false;
    
    // Use 70% radius for tighter hitbox
    float distance = glm::length(position - bulletPos);
    return distance < (radius * 0.7f + bulletRadius);
}

void SmallAsteroid::startBreaking() {
    if (state == State::Normal) {
        state = State::Breaking;
        breakTimer = 0.0f;
        initialScale = scale;
        
        // Increase rotation speed for dramatic effect
        rotationSpeed *= 2.5f;
    }
}

void SmallAsteroid::respawn(const glm::vec3& newPos, const glm::vec3& newVel, float newRadius) {
    position = newPos;
    velocity = newVel;
    radius = newRadius;
    scale = 1.0f;
    initialScale = 1.0f;
    state = State::Normal;
    breakTimer = 0.0f;
    
    // Random rotation for visual variety
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> rotSpeedDist(-60.0f, 60.0f);
    
    rotation = glm::vec3(rotDist(gen), rotDist(gen), rotDist(gen));
    rotationSpeed = glm::vec3(rotSpeedDist(gen), rotSpeedDist(gen), rotSpeedDist(gen));
}

void SmallAsteroid::cleanup() {
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
}

// ============================================================================
// AsteroidFragment Implementation
// ============================================================================

AsteroidFragment::AsteroidFragment(const glm::vec3& startPos, const glm::vec3& vel, float sz)
    : position(startPos), velocity(vel), size(sz), lifetime(0.8f), maxLifetime(0.8f),
      VAO(0), VBO(0) {
    
    // Random rotation
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> rotSpeedDist(-180.0f, 180.0f);
    
    rotation = glm::vec3(rotDist(gen), rotDist(gen), rotDist(gen));
    rotationSpeed = glm::vec3(rotSpeedDist(gen), rotSpeedDist(gen), rotSpeedDist(gen));
    
    // Don't call setupMesh here - will be called on first render
}

AsteroidFragment::~AsteroidFragment() {
    cleanup();
}

// Move constructor
AsteroidFragment::AsteroidFragment(AsteroidFragment&& other) noexcept
    : position(other.position),
      velocity(other.velocity),
      rotation(other.rotation),
      rotationSpeed(other.rotationSpeed),
      size(other.size),
      lifetime(other.lifetime),
      maxLifetime(other.maxLifetime),
      VAO(other.VAO),
      VBO(other.VBO) {
    // Take ownership of OpenGL resources
    other.VAO = 0;
    other.VBO = 0;
}

// Move assignment operator
AsteroidFragment& AsteroidFragment::operator=(AsteroidFragment&& other) noexcept {
    if (this != &other) {
        // Clean up existing resources
        cleanup();
        
        // Move data
        position = other.position;
        velocity = other.velocity;
        rotation = other.rotation;
        rotationSpeed = other.rotationSpeed;
        size = other.size;
        lifetime = other.lifetime;
        maxLifetime = other.maxLifetime;
        VAO = other.VAO;
        VBO = other.VBO;
        
        // Take ownership
        other.VAO = 0;
        other.VBO = 0;
    }
    return *this;
}

void AsteroidFragment::setupMesh() {
    if (VAO != 0) return;  // Already set up
    
    // Simple small cube for fragment
    float s = size * 0.5f;
    float vertices[] = {
        // Front
        -s, -s,  s,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         s, -s,  s,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         s,  s,  s,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         s,  s,  s,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -s,  s,  s,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -s, -s,  s,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
        
        // Back
        -s, -s, -s,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        -s,  s, -s,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         s,  s, -s,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         s,  s, -s,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
         s, -s, -s,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -s, -s, -s,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
        
        // Other faces omitted for brevity (can copy from Bullet.cpp if needed)
        // For fragments, just front/back is acceptable since they're small and fast
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void AsteroidFragment::update(float dt) {
    position += velocity * dt;
    rotation += rotationSpeed * dt;
    lifetime -= dt;
}

glm::mat4 AsteroidFragment::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(size));
    return model;
}

void AsteroidFragment::render(unsigned int textureID) {
    // Lazy initialization
    if (VAO == 0) {
        setupMesh();
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // TODO: Apply alpha fading based on getAlpha()
    // For now, just render normally - alpha can be added with blend mode later
    
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 12);  // 2 faces * 2 triangles * 3 vertices
    glBindVertexArray(0);
}

void AsteroidFragment::cleanup() {
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
}
