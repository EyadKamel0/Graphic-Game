#include "CometRock.h"
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

// PI constant
const float PI = 3.14159265359f;

CometRock::CometRock(const glm::vec3& startPos, const glm::vec3& vel, float rad)
    : position(startPos),
      velocity(vel),
      rotation(0.0f, 0.0f, 0.0f),
      radius(rad),
      scale(1.0f),
      initialScale(1.0f),
      state(State::Normal),
      glowIntensity(2.0f),  // Bright glow by default
      glowPulseTimer(0.0f),
      glowPulseSpeed(4.0f),  // Fast pulsing
      trailTimer(0.0f),
      breakTimer(0.0f),
      breakDuration(0.2f),  // Faster break than asteroids
      VAO(0), VBO(0), EBO(0),
      indexCount(0) {
    
    // Random rotation for variety
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> rotDist(0.0f, 360.0f);
    std::uniform_real_distribution<float> rotSpeedDist(-120.0f, 120.0f);  // Fast rotation
    
    rotation = glm::vec3(rotDist(gen), rotDist(gen), rotDist(gen));
    rotationSpeed = glm::vec3(rotSpeedDist(gen), rotSpeedDist(gen), rotSpeedDist(gen));
    
    setupMesh();
}

CometRock::~CometRock() {
    cleanup();
}

// Move constructor
CometRock::CometRock(CometRock&& other) noexcept
    : position(other.position),
      velocity(other.velocity),
      rotation(other.rotation),
      rotationSpeed(other.rotationSpeed),
      radius(other.radius),
      scale(other.scale),
      initialScale(other.initialScale),
      state(other.state),
      glowIntensity(other.glowIntensity),
      glowPulseTimer(other.glowPulseTimer),
      glowPulseSpeed(other.glowPulseSpeed),
      trailTimer(other.trailTimer),
      breakTimer(other.breakTimer),
      breakDuration(other.breakDuration),
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
CometRock& CometRock::operator=(CometRock&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        position = other.position;
        velocity = other.velocity;
        rotation = other.rotation;
        rotationSpeed = other.rotationSpeed;
        radius = other.radius;
        scale = other.scale;
        initialScale = other.initialScale;
        state = other.state;
        glowIntensity = other.glowIntensity;
        glowPulseTimer = other.glowPulseTimer;
        glowPulseSpeed = other.glowPulseSpeed;
        trailTimer = other.trailTimer;
        breakTimer = other.breakTimer;
        breakDuration = other.breakDuration;
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

void CometRock::setupMesh() {
    if (VAO != 0) return;
    createGlowingSphere(12);  // Medium detail sphere
}

void CometRock::createGlowingSphere(int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    // Generate sphere vertices
    for (int y = 0; y <= segments; y++) {
        for (int x = 0; x <= segments; x++) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)segments;
            
            float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            float yPos = std::cos(ySegment * PI);
            float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
            
            // Position (scaled by radius)
            vertices.push_back(xPos * radius);
            vertices.push_back(yPos * radius);
            vertices.push_back(zPos * radius);
            
            // Normal (same as position direction for sphere)
            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
            
            // Texture coordinates
            vertices.push_back(xSegment);
            vertices.push_back(ySegment);
        }
    }
    
    // Generate indices
    for (int y = 0; y < segments; y++) {
        for (int x = 0; x < segments; x++) {
            unsigned int current = y * (segments + 1) + x;
            unsigned int next = current + segments + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
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

void CometRock::update(float dt) {
    // Update rotation (fast spinning)
    rotation += rotationSpeed * dt;
    
    // Update glow pulse
    glowPulseTimer += dt * glowPulseSpeed;
    glowIntensity = 1.5f + 0.5f * std::sin(glowPulseTimer);  // Pulsing between 1.0 and 2.0
    
    // Update trail timer
    trailTimer += dt;
    
    switch (state) {
        case State::Normal:
            // Move according to velocity (fast!)
            position += velocity * dt;
            break;
            
        case State::Breaking:
            // Breaking animation
            breakTimer += dt;
            if (breakTimer >= breakDuration) {
                state = State::Dead;
            } else {
                // Scale down and increase glow
                float progress = breakTimer / breakDuration;
                scale = initialScale * (1.0f - progress);
                glowIntensity = 2.0f + progress * 3.0f;  // Glow brighter as it breaks
            }
            break;
            
        case State::Dead:
            // Do nothing, ready for removal
            break;
    }
}

bool CometRock::checkCollision(const glm::vec3& bulletPos, float bulletRadius) const {
    if (state != State::Normal) return false;
    
    float distance = glm::length(position - bulletPos);
    return distance < (radius + bulletRadius);
}

bool CometRock::checkPlayerCollision(const glm::vec3& playerPos, float playerRadius) const {
    if (state != State::Normal) return false;
    
    float distance = glm::length(position - playerPos);
    return distance < (radius + playerRadius);
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
    velocity = newVel;
    radius = newRadius;
    scale = 1.0f;
    initialScale = 1.0f;
    state = State::Normal;
    breakTimer = 0.0f;
    glowPulseTimer = 0.0f;
    glowIntensity = 2.0f;
    
    // Randomize rotation
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
    
    // Apply rotations
    model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    model = glm::scale(model, glm::vec3(scale));
    
    return model;
}

void CometRock::render(unsigned int textureID) {
    if (state == State::Dead) return;
    if (VAO == 0 || indexCount == 0) return;
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void CometRock::cleanup() {
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
