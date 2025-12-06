#include "SatelliteFragment.h"
#include "Texture.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <map>

// Static member initialization
std::vector<SatelliteSubMesh> SatelliteFragment::sharedSubMeshes;
std::vector<SatelliteMaterial> SatelliteFragment::sharedMaterials;
bool SatelliteFragment::sharedMeshLoaded = false;

SatelliteFragment::SatelliteFragment(const glm::vec3& startPos, float rotSpeed)
    : position(startPos),
      rotation(0.0f, 0.0f, 0.0f),
      rotationAxis(0.0f, 1.0f, 0.0f),
      rotationSpeed(rotSpeed),
      scale(2.0f),  // Scale up the satellite model
      collisionRadius(3.0f),  // Hitbox proportional to visual (1.5x scale)
      destroyed(false),
      usesSharedMesh(false),
      VAO(0), VBO(0), EBO(0),
      indexCount(0) {
    
    // Random initial rotation for variety
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> axisDist(-1.0f, 1.0f);
    
    rotation = glm::vec3(rotDist(gen), rotDist(gen), rotDist(gen));
    
    // Random rotation axis (normalized)
    rotationAxis = glm::normalize(glm::vec3(axisDist(gen), axisDist(gen), axisDist(gen)));
    if (glm::length(rotationAxis) < 0.1f) {
        rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);  // Fallback
    }
    
    // Random scale for variety
    std::uniform_real_distribution<float> scaleDist(1.5f, 3.0f);
    scale = scaleDist(gen);
    collisionRadius = scale * 2.0f;
    
    setupMesh();
}

SatelliteFragment::~SatelliteFragment() {
    cleanup();
}

// Move constructor
SatelliteFragment::SatelliteFragment(SatelliteFragment&& other) noexcept
    : position(other.position),
      rotation(other.rotation),
      rotationAxis(other.rotationAxis),
      rotationSpeed(other.rotationSpeed),
      scale(other.scale),
      collisionRadius(other.collisionRadius),
      destroyed(other.destroyed),
      usesSharedMesh(other.usesSharedMesh),
      VAO(other.VAO),
      VBO(other.VBO),
      EBO(other.EBO),
      indexCount(other.indexCount) {
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.indexCount = 0;
}

// Move assignment operator
SatelliteFragment& SatelliteFragment::operator=(SatelliteFragment&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        position = other.position;
        rotation = other.rotation;
        rotationAxis = other.rotationAxis;
        rotationSpeed = other.rotationSpeed;
        scale = other.scale;
        collisionRadius = other.collisionRadius;
        destroyed = other.destroyed;
        usesSharedMesh = other.usesSharedMesh;
        VAO = other.VAO;
        VBO = other.VBO;
        EBO = other.EBO;
        indexCount = other.indexCount;
        
        other.VAO = 0;
        other.VBO = 0;
        other.EBO = 0;
        other.indexCount = 0;
    }
    return *this;
}

void SatelliteFragment::loadSharedMesh() {
    if (sharedMeshLoaded) return;
    
    const std::string basePath = "Satellite/";
    const std::string objPath = basePath + "Satellite.obj";
    const std::string mtlPath = basePath + "Satellite.mtl";
    
    loadOBJModel(objPath, mtlPath);
    
    if (!sharedSubMeshes.empty()) {
        sharedMeshLoaded = true;
        std::cout << "[SATELLITE] Loaded shared mesh with " << sharedSubMeshes.size() << " submeshes" << std::endl;
    } else {
        std::cerr << "[SATELLITE] Failed to load OBJ, will use fallback mesh" << std::endl;
    }
}

void SatelliteFragment::cleanupSharedMesh() {
    for (auto& submesh : sharedSubMeshes) {
        if (submesh.VAO != 0) glDeleteVertexArrays(1, &submesh.VAO);
        if (submesh.VBO != 0) glDeleteBuffers(1, &submesh.VBO);
        if (submesh.EBO != 0) glDeleteBuffers(1, &submesh.EBO);
    }
    sharedSubMeshes.clear();
    
    for (auto& material : sharedMaterials) {
        if (material.diffuseTexture != 0) {
            glDeleteTextures(1, &material.diffuseTexture);
        }
        if (material.metallicTexture != 0) {
            glDeleteTextures(1, &material.metallicTexture);
        }
        if (material.roughnessTexture != 0) {
            glDeleteTextures(1, &material.roughnessTexture);
        }
    }
    sharedMaterials.clear();
    
    sharedMeshLoaded = false;
}

void SatelliteFragment::loadOBJModel(const std::string& objPath, const std::string& mtlPath) {
    std::string basePath = objPath.substr(0, objPath.find_last_of("/\\") + 1);
    
    // Load MTL file first
    std::map<std::string, int> materialMap;
    std::ifstream mtlFile(mtlPath);
    if (mtlFile.is_open()) {
        std::string line, currentMat;
        while (std::getline(mtlFile, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;
            
            if (prefix == "newmtl") {
                iss >> currentMat;
                SatelliteMaterial mat;
                mat.name = currentMat;
                mat.diffuseColor = glm::vec3(0.8f);
                mat.diffuseTexture = 0;
                mat.metallicTexture = 0;
                mat.roughnessTexture = 0;
                materialMap[currentMat] = sharedMaterials.size();
                sharedMaterials.push_back(mat);
                
                // Load PBR textures based on material name
                // Textures follow pattern: satellite_<MaterialName>_<Type>.jpg
                std::string texBasePath = basePath + "Textures/satellite_" + currentMat;
                
                // Try loading textures, with fallback for unicode issues (Satélite -> Satellite)
                // Check if material name has non-ASCII characters (unicode issues)
                bool hasUnicode = false;
                for (char c : currentMat) {
                    if (static_cast<unsigned char>(c) > 127) {
                        hasUnicode = true;
                        break;
                    }
                }
                
                std::vector<std::string> tryNames;
                if (hasUnicode) {
                    // For unicode material names, try known fallbacks first
                    tryNames = {"Satellite", "Satelite", currentMat};
                } else {
                    // For normal names, just use the material name
                    tryNames = {currentMat};
                }
                
                bool texturesLoaded = false;
                for (const auto& tryName : tryNames) {
                    if (texturesLoaded) break;
                    
                    std::string tryBasePath = basePath + "Textures/satellite_" + tryName;
                    
                    // BaseColor (albedo) - check if file exists first
                    std::string baseColorPath = tryBasePath + "_BaseColor.jpg";
                    std::ifstream testFile(baseColorPath);
                    if (testFile.good()) {
                        testFile.close();
                        GLuint baseColorTex = loadTexture(baseColorPath);
                        sharedMaterials.back().diffuseTexture = baseColorTex;
                        std::cout << "[SATELLITE] Loaded albedo: " << baseColorPath << std::endl;
                        texturesLoaded = true;
                        
                        // Metallic
                        std::string metallicPath = tryBasePath + "_Metallic.jpg";
                        GLuint metallicTex = loadTexture(metallicPath);
                        sharedMaterials.back().metallicTexture = metallicTex;
                        std::cout << "[SATELLITE] Loaded metallic: " << metallicPath << std::endl;
                        
                        // Roughness
                        std::string roughnessPath = tryBasePath + "_Roughness.jpg";
                        GLuint roughnessTex = loadTexture(roughnessPath);
                        sharedMaterials.back().roughnessTexture = roughnessTex;
                        std::cout << "[SATELLITE] Loaded roughness: " << roughnessPath << std::endl;
                    }
                }
                
                // If we still couldn't load textures, warn
                if (!texturesLoaded) {
                    std::cout << "[SATELLITE] Warning: No textures loaded for material: " << currentMat << std::endl;
                }
            }
            else if (prefix == "Kd" && !currentMat.empty()) {
                int idx = materialMap[currentMat];
                iss >> sharedMaterials[idx].diffuseColor.r 
                    >> sharedMaterials[idx].diffuseColor.g 
                    >> sharedMaterials[idx].diffuseColor.b;
            }
        }
        mtlFile.close();
        std::cout << "[SATELLITE] Loaded " << sharedMaterials.size() << " materials with PBR textures" << std::endl;
    } else {
        std::cout << "[SATELLITE] MTL file not found: " << mtlPath << std::endl;
    }
    
    // Load OBJ file
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;
    
    struct FaceGroup {
        std::string material;
        std::vector<unsigned int> indices;
        std::vector<float> vertices; // pos(3) + normal(3) + uv(2)
    };
    std::map<std::string, FaceGroup> groups;
    std::string currentMaterial = "default";
    
    std::ifstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "[SATELLITE] Failed to load " << objPath << std::endl;
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
            
            auto& group = groups[currentMaterial];
            group.material = currentMaterial;
            
            auto processFaceVertex = [&](const std::string& faceStr) -> unsigned int {
                unsigned int vIdx = 0, vtIdx = 0, vnIdx = 0;
                
                if (sscanf(faceStr.c_str(), "%u/%u/%u", &vIdx, &vtIdx, &vnIdx) != 3) {
                    if (sscanf(faceStr.c_str(), "%u//%u", &vIdx, &vnIdx) != 2) {
                        if (sscanf(faceStr.c_str(), "%u/%u", &vIdx, &vtIdx) != 2) {
                            sscanf(faceStr.c_str(), "%u", &vIdx);
                        }
                    }
                }
                
                unsigned int index = group.vertices.size() / 8;
                
                // Add vertex data: pos(3) + normal(3) + uv(2)
                if (vIdx > 0 && vIdx <= temp_vertices.size()) {
                    group.vertices.push_back(temp_vertices[vIdx-1].x);
                    group.vertices.push_back(temp_vertices[vIdx-1].y);
                    group.vertices.push_back(temp_vertices[vIdx-1].z);
                } else {
                    group.vertices.push_back(0.0f);
                    group.vertices.push_back(0.0f);
                    group.vertices.push_back(0.0f);
                }
                if (vnIdx > 0 && vnIdx <= temp_normals.size()) {
                    group.vertices.push_back(temp_normals[vnIdx-1].x);
                    group.vertices.push_back(temp_normals[vnIdx-1].y);
                    group.vertices.push_back(temp_normals[vnIdx-1].z);
                } else {
                    group.vertices.push_back(0.0f);
                    group.vertices.push_back(1.0f);
                    group.vertices.push_back(0.0f);
                }
                if (vtIdx > 0 && vtIdx <= temp_uvs.size()) {
                    group.vertices.push_back(temp_uvs[vtIdx-1].x);
                    group.vertices.push_back(temp_uvs[vtIdx-1].y);
                } else {
                    group.vertices.push_back(0.0f);
                    group.vertices.push_back(0.0f);
                }
                
                return index;
            };
            
            // Triangulate face
            unsigned int firstIdx = processFaceVertex(faceVerts[0]);
            unsigned int prevIdx = processFaceVertex(faceVerts[1]);
            
            for (size_t i = 2; i < faceVerts.size(); i++) {
                unsigned int currIdx = processFaceVertex(faceVerts[i]);
                group.indices.push_back(firstIdx);
                group.indices.push_back(prevIdx);
                group.indices.push_back(currIdx);
                prevIdx = currIdx;
            }
        }
    }
    objFile.close();
    
    std::cout << "[SATELLITE] Loaded " << temp_vertices.size() << " vertices from OBJ" << std::endl;
    
    // Create submeshes
    for (auto& pair : groups) {
        SatelliteSubMesh submesh;
        auto& group = pair.second;
        
        int matIdx = -1;
        if (materialMap.find(group.material) != materialMap.end()) {
            matIdx = materialMap[group.material];
        }
        submesh.materialIndex = matIdx;
        submesh.indexCount = group.indices.size();
        
        if (group.vertices.empty() || group.indices.empty()) continue;
        
        glGenVertexArrays(1, &submesh.VAO);
        glGenBuffers(1, &submesh.VBO);
        glGenBuffers(1, &submesh.EBO);
        
        glBindVertexArray(submesh.VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, submesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, group.vertices.size() * sizeof(float), 
                     group.vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, submesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, group.indices.size() * sizeof(unsigned int), 
                     group.indices.data(), GL_STATIC_DRAW);
        
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
        
        sharedSubMeshes.push_back(submesh);
    }
    
    std::cout << "[SATELLITE] Created " << sharedSubMeshes.size() << " submeshes" << std::endl;
}

void SatelliteFragment::setupMesh() {
    // Try to use shared mesh
    if (sharedMeshLoaded && !sharedSubMeshes.empty()) {
        usesSharedMesh = true;
        return;
    }
    
    // Fallback to simple mesh if shared mesh not loaded
    createFallbackMesh();
}

void SatelliteFragment::createFallbackMesh() {
    // Create a simple cube as fallback
    float size = 2.0f;
    float hw = size * 0.5f;
    
    float vertices[] = {
        // Top face
        -hw,  hw, -hw,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         hw,  hw, -hw,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         hw,  hw,  hw,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -hw,  hw,  hw,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        
        // Bottom face
        -hw, -hw, -hw,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        -hw, -hw,  hw,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
         hw, -hw,  hw,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         hw, -hw, -hw,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        
        // Front face
        -hw, -hw,  hw,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        -hw,  hw,  hw,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
         hw,  hw,  hw,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
         hw, -hw,  hw,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
        
        // Back face
         hw, -hw, -hw,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         hw,  hw, -hw,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
        -hw,  hw, -hw,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        -hw, -hw, -hw,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
        
        // Right face
         hw, -hw,  hw,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         hw,  hw,  hw,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         hw,  hw, -hw,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         hw, -hw, -hw,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        
        // Left face
        -hw, -hw, -hw,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -hw,  hw, -hw,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -hw,  hw,  hw,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -hw, -hw,  hw,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
    };
    
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };
    
    indexCount = 36;
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
    
    usesSharedMesh = false;
}

void SatelliteFragment::update(float dt) {
    // Continuous rotation around the random axis
    rotation.y += rotationSpeed * dt;
    rotation.x += rotationSpeed * 0.3f * dt;
    rotation.z += rotationSpeed * 0.2f * dt;
}

bool SatelliteFragment::checkCollision(const glm::vec3& targetPos, float targetRadius) const {
    float distance = glm::length(position - targetPos);
    return distance < (collisionRadius + targetRadius);
}

void SatelliteFragment::respawn(const glm::vec3& newPos, float newRotSpeed) {
    position = newPos;
    rotationSpeed = newRotSpeed;
    
    // Randomize rotation again
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    rotation = glm::vec3(rotDist(gen), rotDist(gen), rotDist(gen));
}

glm::mat4 SatelliteFragment::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    
    // Apply rotations
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    model = glm::scale(model, glm::vec3(scale));
    
    return model;
}

void SatelliteFragment::render(unsigned int textureID) {
    if (usesSharedMesh && sharedMeshLoaded) {
        // Render using shared mesh (all submeshes)
        for (const auto& submesh : sharedSubMeshes) {
            // Use material texture if available, otherwise use passed texture
            GLuint texToUse = textureID;
            if (submesh.materialIndex >= 0 && 
                submesh.materialIndex < (int)sharedMaterials.size() &&
                sharedMaterials[submesh.materialIndex].diffuseTexture != 0) {
                texToUse = sharedMaterials[submesh.materialIndex].diffuseTexture;
            }
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texToUse);
            
            glBindVertexArray(submesh.VAO);
            glDrawElements(GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    } else {
        // Render fallback mesh
        if (VAO == 0 || indexCount == 0) return;
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}

void SatelliteFragment::renderWithPBR() {
    if (usesSharedMesh && sharedMeshLoaded) {
        // Render using shared mesh with PBR textures
        for (const auto& submesh : sharedSubMeshes) {
            if (submesh.materialIndex >= 0 && 
                submesh.materialIndex < (int)sharedMaterials.size()) {
                const auto& mat = sharedMaterials[submesh.materialIndex];
                
                // Bind albedo texture (slot 0)
                glActiveTexture(GL_TEXTURE0);
                if (mat.diffuseTexture != 0) {
                    glBindTexture(GL_TEXTURE_2D, mat.diffuseTexture);
                } else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                
                // Bind metallic texture (slot 1)
                glActiveTexture(GL_TEXTURE1);
                if (mat.metallicTexture != 0) {
                    glBindTexture(GL_TEXTURE_2D, mat.metallicTexture);
                } else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                
                // Bind roughness texture (slot 2)
                glActiveTexture(GL_TEXTURE2);
                if (mat.roughnessTexture != 0) {
                    glBindTexture(GL_TEXTURE_2D, mat.roughnessTexture);
                } else {
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
            }
            
            glBindVertexArray(submesh.VAO);
            glDrawElements(GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        // Reset texture units
        glActiveTexture(GL_TEXTURE0);
    } else {
        // Fallback to regular render
        render(0);
    }
}

void SatelliteFragment::cleanup() {
    // Only cleanup fallback mesh (instance-owned)
    // Shared mesh is cleaned up via cleanupSharedMesh()
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
