#include "BlackHolePortal.h"
#include "Texture.h"
#include <cmath>
#include <iostream>
#include <vector>

const float PI = 3.14159265359f;

BlackHolePortal::BlackHolePortal(const glm::vec3& portalPos)
    : position(portalPos),
      state(BlackHoleState::Hidden),
      ringRotation1(0.0f),
      ringRotation2(0.0f),
      ringRotation3(0.0f),
      ringSpeed1(30.0f),
      ringSpeed2(-45.0f),  // Opposite direction
      ringSpeed3(20.0f),
      scale(0.0f),
      targetScale(8.0f),  // Large black hole
      triggerRadius(5.0f),
      spawnAnimTimer(0.0f),
      spawnAnimDuration(2.0f),
      captureTimer(0.0f),
      captureDuration(2.0f),
      captureStrength(25.0f),
      pulseTimer(0.0f),
      pulseSpeed(2.0f),
      glowIntensity(1.0f),
      coreVAO(0), coreVBO(0), coreEBO(0), coreIndexCount(0),
      ring1VAO(0), ring1VBO(0), ring1EBO(0), ring1IndexCount(0),
      ring2VAO(0), ring2VBO(0), ring2EBO(0), ring2IndexCount(0),
      glowVAO(0), glowVBO(0), glowEBO(0), glowIndexCount(0),
      ringTexture(0), lightTexture1(0), lightTexture2(0), lightTexture3(0) {
}

BlackHolePortal::~BlackHolePortal() {
    cleanup();
}

void BlackHolePortal::setupMesh() {
    loadTextures();
    createCoreMesh();
    createRingMesh(ring1VAO, ring1VBO, ring1EBO, ring1IndexCount, 1.5f, 3.0f);  // Inner ring
    createRingMesh(ring2VAO, ring2VBO, ring2EBO, ring2IndexCount, 3.5f, 5.0f);  // Outer ring
    createGlowMesh();
}

void BlackHolePortal::loadTextures() {
    ringTexture = loadTexture("black-hole/textures/blackholering_1_color.png");
    lightTexture1 = loadTexture("black-hole/textures/blackholelight_1_color.png");
    lightTexture2 = loadTexture("black-hole/textures/blackholelight_2_color.png");
    lightTexture3 = loadTexture("black-hole/textures/blackholelight_3_color.png");
    
    if (ringTexture == 0) {
        std::cerr << "[BLACK HOLE] Failed to load ring texture" << std::endl;
    }
    if (lightTexture1 == 0) {
        std::cerr << "[BLACK HOLE] Failed to load light texture 1" << std::endl;
    }
    std::cout << "[BLACK HOLE] Textures loaded" << std::endl;
}

void BlackHolePortal::createCoreMesh() {
    // Create a dark sphere for the black hole center
    const int segments = 32;
    const int rings = 16;
    float radius = 1.2f;
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    for (int r = 0; r <= rings; r++) {
        float phi = PI * (float)r / (float)rings;
        for (int s = 0; s <= segments; s++) {
            float theta = 2.0f * PI * (float)s / (float)segments;
            
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);
            
            // Position
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            // Normal (pointing inward for dark effect)
            vertices.push_back(-x / radius);
            vertices.push_back(-y / radius);
            vertices.push_back(-z / radius);
            // UV
            vertices.push_back((float)s / segments);
            vertices.push_back((float)r / rings);
        }
    }
    
    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < segments; s++) {
            int curr = r * (segments + 1) + s;
            int next = curr + segments + 1;
            
            indices.push_back(curr);
            indices.push_back(next);
            indices.push_back(curr + 1);
            
            indices.push_back(curr + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
    
    coreIndexCount = indices.size();
    
    glGenVertexArrays(1, &coreVAO);
    glGenBuffers(1, &coreVBO);
    glGenBuffers(1, &coreEBO);
    
    glBindVertexArray(coreVAO);
    glBindBuffer(GL_ARRAY_BUFFER, coreVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coreEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void BlackHolePortal::createRingMesh(GLuint& vao, GLuint& vbo, GLuint& ebo, int& indexCount, 
                                      float innerRadius, float outerRadius) {
    const int segments = 64;
    
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * PI * (float)i / (float)segments;
        float c = cos(angle);
        float s = sin(angle);
        
        // Inner vertex
        vertices.push_back(innerRadius * c);
        vertices.push_back(0.0f);
        vertices.push_back(innerRadius * s);
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);  // Normal up
        vertices.push_back((float)i / segments); vertices.push_back(0.0f);  // UV
        
        // Outer vertex
        vertices.push_back(outerRadius * c);
        vertices.push_back(0.0f);
        vertices.push_back(outerRadius * s);
        vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
        vertices.push_back((float)i / segments); vertices.push_back(1.0f);
    }
    
    for (int i = 0; i < segments; i++) {
        int inner = i * 2;
        int outer = i * 2 + 1;
        int nextInner = (i + 1) * 2;
        int nextOuter = (i + 1) * 2 + 1;
        
        // Two triangles per quad
        indices.push_back(inner);
        indices.push_back(outer);
        indices.push_back(nextInner);
        
        indices.push_back(nextInner);
        indices.push_back(outer);
        indices.push_back(nextOuter);
    }
    
    indexCount = indices.size();
    
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void BlackHolePortal::createGlowMesh() {
    // Create a billboard quad for glow effect
    float size = 6.0f;
    
    float vertices[] = {
        // Position          Normal              UV
        -size, -size, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
         size, -size, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         size,  size, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -size,  size, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
    };
    
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
    glowIndexCount = 6;
    
    glGenVertexArrays(1, &glowVAO);
    glGenBuffers(1, &glowVBO);
    glGenBuffers(1, &glowEBO);
    
    glBindVertexArray(glowVAO);
    glBindBuffer(GL_ARRAY_BUFFER, glowVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glowEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void BlackHolePortal::spawn() {
    if (state == BlackHoleState::Hidden) {
        state = BlackHoleState::Activating;
        spawnAnimTimer = 0.0f;
        scale = 0.0f;
        std::cout << "[BLACK HOLE] Spawning..." << std::endl;
    }
}

void BlackHolePortal::startCapture() {
    if (state == BlackHoleState::Active) {
        state = BlackHoleState::Capturing;
        captureTimer = 0.0f;
        std::cout << "[BLACK HOLE] Capturing player!" << std::endl;
    }
}

void BlackHolePortal::update(float dt) {
    // Update rotations
    ringRotation1 += ringSpeed1 * dt;
    ringRotation2 += ringSpeed2 * dt;
    ringRotation3 += ringSpeed3 * dt;
    
    // Pulse effect
    pulseTimer += pulseSpeed * dt;
    glowIntensity = 0.8f + 0.2f * sin(pulseTimer);
    
    switch (state) {
        case BlackHoleState::Hidden:
            break;
            
        case BlackHoleState::Activating:
            spawnAnimTimer += dt;
            {
                float progress = spawnAnimTimer / spawnAnimDuration;
                if (progress >= 1.0f) {
                    progress = 1.0f;
                    state = BlackHoleState::Active;
                    std::cout << "[BLACK HOLE] Active!" << std::endl;
                }
                // Ease-out cubic for smooth growth
                float eased = 1.0f - pow(1.0f - progress, 3.0f);
                scale = targetScale * eased;
                
                // Speed up rotation during spawn
                ringSpeed1 = 30.0f + 60.0f * progress;
                ringSpeed2 = -45.0f - 90.0f * progress;
            }
            break;
            
        case BlackHoleState::Active:
            // Gentle pulsing
            scale = targetScale * (1.0f + 0.02f * sin(pulseTimer * 2.0f));
            break;
            
        case BlackHoleState::Capturing:
            captureTimer += dt;
            {
                // Capture completes when player reaches center (set externally)
                // Just increase rotation during capture for visual effect
                float progress = std::min(captureTimer / captureDuration, 1.0f);
                ringSpeed1 = 90.0f + 180.0f * progress;
                ringSpeed2 = -135.0f - 270.0f * progress;
                ringSpeed3 = 60.0f + 120.0f * progress;
            }
            break;
            
        case BlackHoleState::Completed:
            break;
    }
}

bool BlackHolePortal::checkPlayerEntry(const glm::vec3& playerPos, float playerRadius) {
    if (state != BlackHoleState::Active) return false;
    
    float distance = glm::length(position - playerPos);
    return distance < (triggerRadius + playerRadius);
}

glm::vec3 BlackHolePortal::pullPlayerToward(const glm::vec3& playerPos, float dt) {
    if (state != BlackHoleState::Capturing) return playerPos;
    
    glm::vec3 toCenter = position - playerPos;
    float distance = glm::length(toCenter);
    
    // Freeze when player is about 10 units from center (inside the dark visual area)
    if (distance < 10.5f) {
        // Player is in the dark center area - complete capture and freeze here
        state = BlackHoleState::Completed;
        std::cout << "[BLACK HOLE] Capture complete! Frozen at distance: " << distance << std::endl;
        return playerPos;  // Keep player at current position, don't move further
    }
    
    glm::vec3 direction = glm::normalize(toCenter);
    // Stronger pull, accelerating as player gets closer
    float pullStrength = captureStrength * (1.0f + captureTimer / captureDuration) * (1.0f + 5.0f / distance);
    
    return playerPos + direction * pullStrength * dt;
}

glm::mat4 BlackHolePortal::getModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(scale));
    return model;
}

void BlackHolePortal::render(unsigned int shaderProgram) {
    if (state == BlackHoleState::Hidden) return;
    if (scale < 0.01f) return;
    
    // Enable blending for glow effects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Get shader uniform location
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    
    // Render outer ring (ring 2)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ringTexture);
    
    glm::mat4 ring2Model = glm::mat4(1.0f);
    ring2Model = glm::translate(ring2Model, position);
    ring2Model = glm::rotate(ring2Model, glm::radians(ringRotation2), glm::vec3(0.0f, 1.0f, 0.0f));
    ring2Model = glm::rotate(ring2Model, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // Tilt
    ring2Model = glm::scale(ring2Model, glm::vec3(scale));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &ring2Model[0][0]);
    
    glBindVertexArray(ring2VAO);
    glDrawElements(GL_TRIANGLES, ring2IndexCount, GL_UNSIGNED_INT, 0);
    
    // Render inner ring (ring 1)
    glBindTexture(GL_TEXTURE_2D, lightTexture2);
    
    glm::mat4 ring1Model = glm::mat4(1.0f);
    ring1Model = glm::translate(ring1Model, position);
    ring1Model = glm::rotate(ring1Model, glm::radians(ringRotation1), glm::vec3(0.0f, 1.0f, 0.0f));
    ring1Model = glm::rotate(ring1Model, glm::radians(-10.0f), glm::vec3(0.0f, 0.0f, 1.0f));  // Different tilt
    ring1Model = glm::scale(ring1Model, glm::vec3(scale));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &ring1Model[0][0]);
    
    glBindVertexArray(ring1VAO);
    glDrawElements(GL_TRIANGLES, ring1IndexCount, GL_UNSIGNED_INT, 0);
    
    // Render dark core last
    glBindTexture(GL_TEXTURE_2D, 0);  // No texture, pure dark
    
    glm::mat4 coreModel = glm::mat4(1.0f);
    coreModel = glm::translate(coreModel, position);
    coreModel = glm::scale(coreModel, glm::vec3(scale));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &coreModel[0][0]);
    
    // Set dark color for core
    int objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
    if (objectColorLoc != -1) {
        glUniform3f(objectColorLoc, 0.02f, 0.02f, 0.05f);  // Almost black with hint of blue
    }
    
    glBindVertexArray(coreVAO);
    glDrawElements(GL_TRIANGLES, coreIndexCount, GL_UNSIGNED_INT, 0);
    
    // Reset color
    if (objectColorLoc != -1) {
        glUniform3f(objectColorLoc, 1.0f, 1.0f, 1.0f);
    }
    
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

void BlackHolePortal::cleanup() {
    auto cleanupMesh = [](GLuint& vao, GLuint& vbo, GLuint& ebo) {
        if (vao != 0) { glDeleteVertexArrays(1, &vao); vao = 0; }
        if (vbo != 0) { glDeleteBuffers(1, &vbo); vbo = 0; }
        if (ebo != 0) { glDeleteBuffers(1, &ebo); ebo = 0; }
    };
    
    cleanupMesh(coreVAO, coreVBO, coreEBO);
    cleanupMesh(ring1VAO, ring1VBO, ring1EBO);
    cleanupMesh(ring2VAO, ring2VBO, ring2EBO);
    cleanupMesh(glowVAO, glowVBO, glowEBO);
    
    if (ringTexture != 0) { glDeleteTextures(1, &ringTexture); ringTexture = 0; }
    if (lightTexture1 != 0) { glDeleteTextures(1, &lightTexture1); lightTexture1 = 0; }
    if (lightTexture2 != 0) { glDeleteTextures(1, &lightTexture2); lightTexture2 = 0; }
    if (lightTexture3 != 0) { glDeleteTextures(1, &lightTexture3); lightTexture3 = 0; }
}
