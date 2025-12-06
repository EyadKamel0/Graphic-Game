#include "WavePortal.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

// ═══════════════════════════════════════════════════════════════
//  LEVEL 1 PORTAL - Size & Animation Constants
// ═══════════════════════════════════════════════════════════════
// LARGE, impressive portal that's clearly visible from distance
const float PORTAL_SCALE = 8.0f;              // Base size (LARGE - clearly visible)
const float PORTAL_ENTRY_RADIUS = 6.0f;       // Collision radius (ship can pass through)
const float PORTAL_ROTATE_SPEED_INACTIVE = 0.0f;     // Hidden = no rotation
const float PORTAL_ROTATE_SPEED_ACTIVE = 120.0f;     // Fast active rotation
const float PORTAL_SPAWN_DURATION = 1.5f;     // Spawn animation time
const float PORTAL_CAPTURE_DURATION = 1.5f;   // Suck-in effect time

WavePortal::WavePortal(const glm::vec3& portalPos)
    : position(portalPos),
      state(PortalState::Hidden),  // Start hidden
      rotation(0.0f),
      rotationSpeed(0.0f),  // No rotation when hidden
      targetRotationSpeed(0.0f),
      scale(0.0f),  // Start at zero scale (invisible)
      targetScale(0.0f),
      triggerRadius(PORTAL_ENTRY_RADIUS),
      active(false),
      playerInside(false),
      spawnAnimTimer(0.0f),
      spawnAnimDuration(PORTAL_SPAWN_DURATION),
      captureTimer(0.0f),
      captureDuration(PORTAL_CAPTURE_DURATION),
      captureStrength(8.0f),  // Pull strength
      activationTimer(0.0f),
      activationDuration(1.5f),
      activationBurstTimer(0.0f),
      pulseTimer(0.0f),
      pulseSpeed(2.0f),
      lightIntensity(0.0f),  // Dark when hidden
      targetLightIntensity(0.0f),
      proximityGlow(0.0f),
      VAO(0), VBO(0), EBO(0),
      indexCount(0) {
}

WavePortal::~WavePortal() {
    cleanup();
}

void WavePortal::setupMesh() {
    if (VAO != 0) return;  // Already set up
    
    loadPortalOBJ();
}

void WavePortal::loadPortalOBJ() {
    // Load the Dr. Strange portal OBJ model
    const std::string objPath = "portal/dc.strange portal.obj";
    
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    std::ifstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "[PORTAL] Failed to load portal OBJ: " << objPath << std::endl;
        // Fallback to simple ring if OBJ not found
        createRingMesh();
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
    
    std::cout << "[PORTAL] Loaded dc.strange portal.obj: " << indices.size() / 3 << " triangles" << std::endl;
}

void WavePortal::createRingMesh() {
    // Create a ring/torus shape (simplified as a thick circle)
    const int segments = 32;
    const float outerRadius = 1.0f;
    const float innerRadius = 0.7f;
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Create outer and inner circle vertices
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * 2.0f * 3.14159f;
        float cosA = cos(angle);
        float sinA = sin(angle);
        
        // Outer vertex
        vertices.push_back(cosA * outerRadius);  // x
        vertices.push_back(sinA * outerRadius);  // y
        vertices.push_back(0.0f);                // z
        vertices.push_back(0.0f);                // nx
        vertices.push_back(0.0f);                // ny
        vertices.push_back(1.0f);                // nz
        vertices.push_back((float)i / segments); // u
        vertices.push_back(1.0f);                // v
        
        // Inner vertex
        vertices.push_back(cosA * innerRadius);
        vertices.push_back(sinA * innerRadius);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
        vertices.push_back((float)i / segments);
        vertices.push_back(0.0f);
    }
    
    // Create triangle strip indices
    for (int i = 0; i < segments; ++i) {
        int outer1 = i * 2;
        int inner1 = i * 2 + 1;
        int outer2 = (i + 1) * 2;
        int inner2 = (i + 1) * 2 + 1;
        
        // First triangle
        indices.push_back(outer1);
        indices.push_back(inner1);
        indices.push_back(outer2);
        
        // Second triangle
        indices.push_back(outer2);
        indices.push_back(inner1);
        indices.push_back(inner2);
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

void WavePortal::update(float dt) {
    /*
     * ═══════════════════════════════════════════════════════════════
     *  PORTAL STATE MACHINE UPDATE
     * ═══════════════════════════════════════════════════════════════
     * Hidden: scale=0, invisible
     * Activating: grow from 0→8, light 0→bright, start spinning
     * Active: full size, pulsing glow, spinning clockwise
     * Capturing: pull player in
     * Completed: done
     */
    
    // Rotate clockwise around Z-axis (facing player) - negative for clockwise
    rotation -= rotationSpeed * dt;
    if (rotation < 0.0f) rotation += 360.0f;
    
    // Smoothly interpolate rotation speed
    float speedLerpRate = 2.0f;
    rotationSpeed = glm::mix(rotationSpeed, targetRotationSpeed, speedLerpRate * dt);
    
    // Smoothly interpolate scale
    float scaleLerpRate = 3.0f;
    scale = glm::mix(scale, targetScale, scaleLerpRate * dt);
    
    // Smoothly interpolate light intensity
    float lightLerpRate = 2.0f;
    lightIntensity = glm::mix(lightIntensity, targetLightIntensity, lightLerpRate * dt);
    
    // State-specific behavior
    switch (state) {
        case PortalState::Hidden:
            // Invisible, waiting for spawn trigger
            targetRotationSpeed = 0.0f;
            targetScale = 0.0f;
            targetLightIntensity = 0.0f;
            break;
            
        case PortalState::Activating: {
            // Spawn animation: grow from 0 to full size
            spawnAnimTimer += dt;
            if (spawnAnimTimer >= spawnAnimDuration) {
                // Animation complete
                state = PortalState::Active;
                active = true;
                std::cout << "[PORTAL] 🌀 Fully materialized and ACTIVE!" << std::endl;
            } else {
                // Animate growth
                float progress = spawnAnimTimer / spawnAnimDuration;
                float easedProgress = sin(progress * 1.5708f);  // Ease-out
                
                targetScale = PORTAL_SCALE * easedProgress;
                targetRotationSpeed = PORTAL_ROTATE_SPEED_ACTIVE * easedProgress;
                targetLightIntensity = 1.5f * easedProgress;
            }
            break;
        }
            
        case PortalState::Active: {
            // Full size, fast rotation, pulsing glow
            targetRotationSpeed = PORTAL_ROTATE_SPEED_ACTIVE;
            targetScale = PORTAL_SCALE;
            
            // Pulsing light effect (breathing)
            pulseTimer += pulseSpeed * dt;
            float pulseMod = sin(pulseTimer) * 0.3f;
            targetLightIntensity = 1.2f + pulseMod;  // 0.9 to 1.5
            
            // Subtle scale breathing
            float scaleModulation = 0.1f * sin(pulseTimer * 4.0f);
            targetScale = PORTAL_SCALE + scaleModulation;
            
            // React more intensely when player is inside trigger area
            if (playerInside) {
                targetRotationSpeed = PORTAL_ROTATE_SPEED_ACTIVE * 3.5f;  // Spin MUCH faster
                targetLightIntensity = 3.5f + pulseMod;  // Glow MUCH brighter
                targetScale = PORTAL_SCALE * 1.15f;  // Grow slightly
            }
            break;
        }
            
        case PortalState::Capturing: {
            // Suck-in effect: increase rotation, maintain size
            captureTimer += dt;
            if (captureTimer >= captureDuration) {
                state = PortalState::Completed;
                std::cout << "[PORTAL] ✨ Player absorbed! Level complete!" << std::endl;
            } else {
                // Ramp up rotation and light as player is pulled in
                float captureProgress = captureTimer / captureDuration;
                targetRotationSpeed = PORTAL_ROTATE_SPEED_ACTIVE * (1.0f + captureProgress * 2.0f);
                targetLightIntensity = 2.0f + captureProgress;  // Get brighter
                targetScale = PORTAL_SCALE * (1.0f + captureProgress * 0.5f);  // Slight growth
            }
            break;
        }
            
        case PortalState::Completed:
            // Done, maintain final state
            break;
    }
}

void WavePortal::spawn() {
    if (state != PortalState::Hidden) return;
    
    state = PortalState::Activating;
    spawnAnimTimer = 0.0f;
    
    std::cout << "[PORTAL] Portal materializing..." << std::endl;
}

void WavePortal::activate() {
    // Legacy method - now calls spawn
    spawn();
}

void WavePortal::startCapture() {
    if (state != PortalState::Active) return;
    
    state = PortalState::Capturing;
    captureTimer = 0.0f;
    
    std::cout << "[PORTAL] Pulling player in..." << std::endl;
}

glm::vec3 WavePortal::pullPlayerToward(const glm::vec3& playerPos, float dt) {
    // Smoothly interpolate player position toward portal center
    float captureProgress = captureTimer / captureDuration;
    float pullSpeed = captureStrength * (1.0f + captureProgress * 2.0f);  // Accelerate
    
    return glm::mix(playerPos, position, pullSpeed * dt);
}

bool WavePortal::checkPlayerEntry(const glm::vec3& playerPos, float playerRadius) {
    if (state != PortalState::Active) return false;
    
    float distance = glm::length(position - playerPos);
    bool nowInside = distance < (triggerRadius + playerRadius);
    
    // Trigger on entry (not while staying inside)
    if (nowInside && !playerInside) {
        playerInside = true;
        return true;  // Signal entry detected
    }
    
    if (!nowInside) {
        playerInside = false;
    }
    
    return false;
}

glm::mat4 WavePortal::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    
    // Rotate clockwise around Z-axis (the circle spins facing the player)
    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    
    model = glm::scale(model, glm::vec3(scale));
    
    return model;
}

void WavePortal::render(unsigned int textureID) {
    // Skip rendering if hidden or mesh not initialized
    if (state == PortalState::Hidden || VAO == 0 || indexCount == 0 || scale < 0.01f) {
        return;  // Invisible
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // TODO: Set lightIntensity as shader uniform for glow effect
    // shader.setFloat("emissiveIntensity", lightIntensity);
}

void WavePortal::playActivationSound() {
    // TODO: Implement audio system
    // soundEngine->playSound("portal_activate.wav");
}

void WavePortal::cleanup() {
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
