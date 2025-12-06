#include "SwingSolarPanel.h"
#include "Texture.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

// PI constant
const float PI = 3.14159265359f;

// Static member initialization
std::vector<SolarPanelSubMesh> SwingSolarPanel::sharedSubMeshes;
std::vector<SolarPanelMaterial> SwingSolarPanel::sharedMaterials;
bool SwingSolarPanel::sharedMeshLoaded = false;

SwingSolarPanel::SwingSolarPanel(const glm::vec3& pivotPos, float swingAng, float swingSpd)
    : pivotPosition(pivotPos),
      swingAngle(swingAng),
      swingSpeed(swingSpd),
      currentSwingAngle(0.0f),
      swingTimer(0.0f),
      panelLength(6.0f),
      scale(0.08f),  // Slightly larger scale for visibility
      collisionRadius(5.0f),  // Larger hitbox to match visual size
      destroyed(false),
      usesSharedMesh(false),
      VAO(0), VBO(0), EBO(0),
      indexCount(0) {
    
    // Random starting phase for variety
    static int panelCount = 0;
    swingTimer = (panelCount++ % 4) * 1.57f;  // Stagger phases by 90 degrees
    
    setupMesh();
}

SwingSolarPanel::~SwingSolarPanel() {
    cleanup();
}

// Move constructor
SwingSolarPanel::SwingSolarPanel(SwingSolarPanel&& other) noexcept
    : pivotPosition(other.pivotPosition),
      swingAngle(other.swingAngle),
      swingSpeed(other.swingSpeed),
      currentSwingAngle(other.currentSwingAngle),
      swingTimer(other.swingTimer),
      panelLength(other.panelLength),
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
SwingSolarPanel& SwingSolarPanel::operator=(SwingSolarPanel&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        pivotPosition = other.pivotPosition;
        swingAngle = other.swingAngle;
        swingSpeed = other.swingSpeed;
        currentSwingAngle = other.currentSwingAngle;
        swingTimer = other.swingTimer;
        panelLength = other.panelLength;
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

void SwingSolarPanel::loadSharedMesh() {
    if (sharedMeshLoaded) return;
    
    const std::string basePath = "SolarPanel/";
    const std::string objPath = basePath + "10781_Solar-Panels_V1.obj";
    const std::string mtlPath = basePath + "10781_Solar-Panels_V1.mtl";
    
    loadOBJModel(objPath, mtlPath);
    
    if (!sharedSubMeshes.empty()) {
        sharedMeshLoaded = true;
        std::cout << "[SOLAR PANEL] Loaded shared mesh with " << sharedSubMeshes.size() << " submeshes" << std::endl;
    } else {
        std::cerr << "[SOLAR PANEL] Failed to load OBJ, will use fallback mesh" << std::endl;
    }
}

void SwingSolarPanel::cleanupSharedMesh() {
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

void SwingSolarPanel::loadOBJModel(const std::string& objPath, const std::string& mtlPath) {
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
                SolarPanelMaterial mat;
                mat.name = currentMat;
                mat.diffuseColor = glm::vec3(0.8f);
                mat.diffuseTexture = 0;
                mat.metallicTexture = 0;
                mat.roughnessTexture = 0;
                materialMap[currentMat] = sharedMaterials.size();
                sharedMaterials.push_back(mat);
            }
            else if (prefix == "Kd" && !currentMat.empty()) {
                int idx = materialMap[currentMat];
                iss >> sharedMaterials[idx].diffuseColor.r 
                    >> sharedMaterials[idx].diffuseColor.g 
                    >> sharedMaterials[idx].diffuseColor.b;
            }
            else if (prefix == "map_Kd" && !currentMat.empty()) {
                std::string texFile;
                iss >> texFile;
                int idx = materialMap[currentMat];
                std::string texPath = basePath + texFile;
                // Check if file exists before loading
                std::ifstream testFile(texPath);
                if (testFile.good()) {
                    testFile.close();
                    sharedMaterials[idx].diffuseTexture = loadTexture(texPath);
                    std::cout << "[SOLAR PANEL] Loaded albedo: " << texPath << std::endl;
                }
            }
            else if (prefix == "map_refl" && !currentMat.empty()) {
                // Metallic map
                std::string texFile;
                iss >> texFile;
                int idx = materialMap[currentMat];
                std::string texPath = basePath + texFile;
                std::ifstream testFile(texPath);
                if (testFile.good()) {
                    testFile.close();
                    sharedMaterials[idx].metallicTexture = loadTexture(texPath);
                    std::cout << "[SOLAR PANEL] Loaded metallic: " << texPath << std::endl;
                }
            }
            else if (prefix == "map_Ns" && !currentMat.empty()) {
                // Roughness map
                std::string texFile;
                iss >> texFile;
                int idx = materialMap[currentMat];
                std::string texPath = basePath + texFile;
                std::ifstream testFile(texPath);
                if (testFile.good()) {
                    testFile.close();
                    sharedMaterials[idx].roughnessTexture = loadTexture(texPath);
                    std::cout << "[SOLAR PANEL] Loaded roughness: " << texPath << std::endl;
                }
            }
        }
        mtlFile.close();
        std::cout << "[SOLAR PANEL] Loaded " << sharedMaterials.size() << " materials from MTL" << std::endl;
        
        // Find the first material with textures and share with materials that don't have any
        GLuint sharedAlbedo = 0, sharedMetallic = 0, sharedRoughness = 0;
        for (const auto& mat : sharedMaterials) {
            if (mat.diffuseTexture != 0 && sharedAlbedo == 0) sharedAlbedo = mat.diffuseTexture;
            if (mat.metallicTexture != 0 && sharedMetallic == 0) sharedMetallic = mat.metallicTexture;
            if (mat.roughnessTexture != 0 && sharedRoughness == 0) sharedRoughness = mat.roughnessTexture;
        }
        
        // Apply shared textures to materials without textures
        for (auto& mat : sharedMaterials) {
            if (mat.diffuseTexture == 0 && sharedAlbedo != 0) {
                mat.diffuseTexture = sharedAlbedo;
                std::cout << "[SOLAR PANEL] Shared albedo texture to material: " << mat.name << std::endl;
            }
            if (mat.metallicTexture == 0 && sharedMetallic != 0) {
                mat.metallicTexture = sharedMetallic;
            }
            if (mat.roughnessTexture == 0 && sharedRoughness != 0) {
                mat.roughnessTexture = sharedRoughness;
            }
        }
    } else {
        std::cout << "[SOLAR PANEL] MTL file not found: " << mtlPath << std::endl;
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
        std::cerr << "[SOLAR PANEL] Failed to load " << objPath << std::endl;
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
    
    std::cout << "[SOLAR PANEL] Loaded " << temp_vertices.size() << " vertices from OBJ" << std::endl;
    
    // Create submeshes
    for (auto& pair : groups) {
        SolarPanelSubMesh submesh;
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
    
    std::cout << "[SOLAR PANEL] Created " << sharedSubMeshes.size() << " submeshes" << std::endl;
}

void SwingSolarPanel::setupMesh() {
    // Try to use shared mesh
    if (sharedMeshLoaded && !sharedSubMeshes.empty()) {
        usesSharedMesh = true;
        return;
    }
    
    // Fallback to simple mesh if shared mesh not loaded
    createFallbackMesh();
}

void SwingSolarPanel::createFallbackMesh() {
    // Create a large rectangular panel as fallback
    float hw = 4.0f;
    float hh = 0.2f;
    float hd = 2.0f;
    float yOffset = -panelLength;
    
    float vertices[] = {
        // Top face
        -hw,  hh + yOffset, -hd,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         hw,  hh + yOffset, -hd,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         hw,  hh + yOffset,  hd,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -hw,  hh + yOffset,  hd,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        
        // Bottom face
        -hw, -hh + yOffset, -hd,  0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
        -hw, -hh + yOffset,  hd,  0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
         hw, -hh + yOffset,  hd,  0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
         hw, -hh + yOffset, -hd,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        
        // Front face
        -hw, -hh + yOffset,  hd,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        -hw,  hh + yOffset,  hd,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
         hw,  hh + yOffset,  hd,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
         hw, -hh + yOffset,  hd,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
        
        // Back face
         hw, -hh + yOffset, -hd,  0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
         hw,  hh + yOffset, -hd,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f,
        -hw,  hh + yOffset, -hd,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
        -hw, -hh + yOffset, -hd,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
        
        // Right face
         hw, -hh + yOffset,  hd,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         hw,  hh + yOffset,  hd,  1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
         hw,  hh + yOffset, -hd,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
         hw, -hh + yOffset, -hd,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        
        // Left face
        -hw, -hh + yOffset, -hd,  -1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
        -hw,  hh + yOffset, -hd,  -1.0f, 0.0f, 0.0f,  0.0f, 1.0f,
        -hw,  hh + yOffset,  hd,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
        -hw, -hh + yOffset,  hd,  -1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
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

void SwingSolarPanel::update(float dt) {
    // Pendulum motion using sine wave
    swingTimer += dt * swingSpeed;
    currentSwingAngle = swingAngle * std::sin(swingTimer * 2.0f);
}

glm::vec3 SwingSolarPanel::getPanelCenterPosition() const {
    // Calculate the actual center position of the panel based on swing
    float angleRad = glm::radians(currentSwingAngle);
    
    // Panel hangs below pivot, offset by panelLength
    float xOffset = std::sin(angleRad) * panelLength;
    float yOffset = -std::cos(angleRad) * panelLength;
    
    return pivotPosition + glm::vec3(xOffset, yOffset, 0.0f);
}

bool SwingSolarPanel::checkCollision(const glm::vec3& targetPos, float targetRadius) const {
    glm::vec3 panelCenter = getPanelCenterPosition();
    float distance = glm::length(panelCenter - targetPos);
    return distance < (collisionRadius + targetRadius);
}

void SwingSolarPanel::respawn(const glm::vec3& newPivotPos, float newSwingAngle, float newSwingSpeed) {
    pivotPosition = newPivotPos;
    swingAngle = newSwingAngle;
    swingSpeed = newSwingSpeed;
    swingTimer = 0.0f;
    currentSwingAngle = 0.0f;
}

glm::mat4 SwingSolarPanel::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    
    // Translate to pivot position
    model = glm::translate(model, pivotPosition);
    
    // Rotate around Z-axis for pendulum swing (left-right)
    model = glm::rotate(model, glm::radians(currentSwingAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Rotate 90 degrees on X to orient the panel properly (facing camera)
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Scale
    model = glm::scale(model, glm::vec3(scale));
    
    return model;
}

void SwingSolarPanel::render(unsigned int textureID) {
    if (usesSharedMesh && sharedMeshLoaded) {
        // Render using shared mesh (all submeshes) with PBR textures
        for (const auto& submesh : sharedSubMeshes) {
            const SolarPanelMaterial* mat = nullptr;
            if (submesh.materialIndex >= 0 && 
                submesh.materialIndex < (int)sharedMaterials.size()) {
                mat = &sharedMaterials[submesh.materialIndex];
            }
            
            // Bind albedo/diffuse texture to slot 0
            glActiveTexture(GL_TEXTURE0);
            if (mat && mat->diffuseTexture != 0) {
                glBindTexture(GL_TEXTURE_2D, mat->diffuseTexture);
            } else {
                glBindTexture(GL_TEXTURE_2D, textureID);
            }
            
            // Bind metallic texture to slot 1
            glActiveTexture(GL_TEXTURE1);
            if (mat && mat->metallicTexture != 0) {
                glBindTexture(GL_TEXTURE_2D, mat->metallicTexture);
            } else {
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            
            // Bind roughness texture to slot 2
            glActiveTexture(GL_TEXTURE2);
            if (mat && mat->roughnessTexture != 0) {
                glBindTexture(GL_TEXTURE_2D, mat->roughnessTexture);
            } else {
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            
            glBindVertexArray(submesh.VAO);
            glDrawElements(GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
        
        // Reset active texture
        glActiveTexture(GL_TEXTURE0);
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

void SwingSolarPanel::renderWithPBR() {
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

// Static method to get material info for PBR shader setup
const std::vector<SolarPanelMaterial>& SwingSolarPanel::getMaterials() {
    return sharedMaterials;
}

const std::vector<SolarPanelSubMesh>& SwingSolarPanel::getSubMeshes() {
    return sharedSubMeshes;
}

void SwingSolarPanel::cleanup() {
    // Only cleanup fallback mesh (instance-owned)
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
