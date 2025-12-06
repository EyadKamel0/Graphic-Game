#include "Asteroid.h"
#include <glad/glad.h>
#include <glm/gtc/constants.hpp>
#include <vector>
#include <cmath>
#include <random>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

Asteroid::Asteroid(const glm::vec3& startPos, const glm::vec3& vel, float rad)
    : position(startPos), velocity(vel), radius(rad), 
      VAO(0), VBO(0), EBO(0), indexCount(0),
      movementBoundsX(40.0f), movementBoundsZ(180.0f) {
    
    // Visual scale is slightly larger than collision radius for variety
    scale = radius * 1.2f;
    
    // Give each asteroid a random rotation and rotation speed for visual variety
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> rotSpeedDist(-30.0f, 30.0f);
    
    rotation = glm::vec3(rotDist(gen), rotDist(gen), rotDist(gen));
    rotationSpeed = glm::vec3(rotSpeedDist(gen), rotSpeedDist(gen), rotSpeedDist(gen));
    
    setupMesh();
}

Asteroid::~Asteroid() {
    cleanup();
}

void Asteroid::setupMesh() {
    // Load the Asteroid_1e.obj model
    const std::string objPath = "Asteroid/Asteroid_1e.obj";
    
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    std::ifstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "Failed to load asteroid OBJ: " << objPath << std::endl;
        // Fallback to sphere if OBJ not found
        createSphereMesh(16);
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
            
            // Triangulate face (fan triangulation)
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
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    std::cout << "Loaded Asteroid_1e.obj: " << indices.size() / 3 << " triangles" << std::endl;
}

void Asteroid::createSphereMesh(int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Generate sphere vertices
    // Format: position (3), normal (3), texcoord (2)
    for (int lat = 0; lat <= segments; ++lat) {
        float theta = lat * glm::pi<float>() / segments;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);
        
        for (int lon = 0; lon <= segments; ++lon) {
            float phi = lon * 2.0f * glm::pi<float>() / segments;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);
            
            // Position (on unit sphere, will be scaled later)
            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;
            
            // Normal (same as position for sphere)
            float nx = x;
            float ny = y;
            float nz = z;
            
            // Texture coordinates
            float u = (float)lon / segments;
            float v = (float)lat / segments;
            
            vertices.insert(vertices.end(), {
                x, y, z,        // Position
                nx, ny, nz,     // Normal
                u, v            // TexCoord
            });
        }
    }
    
    // Generate indices for triangles
    for (int lat = 0; lat < segments; ++lat) {
        for (int lon = 0; lon < segments; ++lon) {
            int first = lat * (segments + 1) + lon;
            int second = first + segments + 1;
            
            // First triangle
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            // Second triangle
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    indexCount = indices.size();
    
    // Create OpenGL buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void Asteroid::update(float dt) {
    // Move asteroid along its velocity vector
    position += velocity * dt;
    
    // Apply rotation for visual interest
    rotation += rotationSpeed * dt;
    
    // Wrap rotation to [0, 360] to prevent overflow
    rotation.x = fmod(rotation.x, 360.0f);
    rotation.y = fmod(rotation.y, 360.0f);
    rotation.z = fmod(rotation.z, 360.0f);
    
    // Keep asteroids within movement bounds (wrap around)
    if (position.x < -movementBoundsX) position.x = movementBoundsX;
    if (position.x > movementBoundsX) position.x = -movementBoundsX;
    if (position.z < -movementBoundsZ) position.z = movementBoundsZ;
    if (position.z > movementBoundsZ) position.z = -movementBoundsZ;
}

glm::mat4 Asteroid::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    
    // Apply rotation on all axes for tumbling effect
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Scale to visual size
    model = glm::scale(model, glm::vec3(scale));
    
    return model;
}

void Asteroid::render(unsigned int textureID) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool Asteroid::checkCollision(const glm::vec3& otherPos, float otherRadius) const {
    // Skip if destroyed
    if (destroyed) return false;
    
    // Simple sphere-sphere collision detection
    // Use 70% of radius for collision detection
    float distance = glm::length(position - otherPos);
    return distance < (radius * 0.7f + otherRadius);
}

void Asteroid::cleanup() {
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
