#include "PlayerShip.h"
#include "Shader.h"
#include "Texture.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

/*
 * ═══════════════════════════════════════════════════════════════
 *  LEVEL 1: OUTER DRIFT ZONE - MOVEMENT CONFIGURATION
 * ═══════════════════════════════════════════════════════════════
 * 
 * These constants define the "fake 3D" on-rails play area for Level 1.
 * Ship moves in X/Y plane while world scrolls along -Z at constant speed.
 * 
 * Tuning Guide:
 *   - Increase L1_SHIP_MOVE_SPEED for faster, more responsive movement
 *   - Increase L1_SHIP_ACCEL for snappier starts/stops (less floaty)
 *   - Expand play area bounds for more freedom of movement
 */

// Level 1 ship movement speed (units per second in X/Y plane)
const float L1_SHIP_MOVE_SPEED = 25.0f;     // Base lateral movement speed

// Level 1 movement smoothing (arcade feel - not too floaty, not too snappy)
const float L1_SHIP_ACCEL = 12.0f;          // How fast ship reaches target velocity

// Level 1 fake 3D playable area boundaries (ship clamped within these)
const float L1_PLAY_AREA_MIN_X = -50.0f;    // Left boundary
const float L1_PLAY_AREA_MAX_X =  50.0f;    // Right boundary  
const float L1_PLAY_AREA_MIN_Y = -30.0f;    // Bottom boundary
const float L1_PLAY_AREA_MAX_Y =  30.0f;    // Top boundary

PlayerShip::PlayerShip() 
    : position(0.0f, 0.0f, -10.0f),  // Start at fixed Z position
      velocity(0.0f, 0.0f, 0.0f),
      moveSpeed(L1_SHIP_MOVE_SPEED),
      moveDamping(10.0f),
      moveAccel(L1_SHIP_ACCEL),
      boundMinX(L1_PLAY_AREA_MIN_X),
      boundMaxX(L1_PLAY_AREA_MAX_X),
      boundMinY(L1_PLAY_AREA_MIN_Y),
      boundMaxY(L1_PLAY_AREA_MAX_Y),
      fixedZ(-10.0f),
      bankAngle(0.0f),
      maxBankAngle(25.0f),
      bankSpeed(120.0f),
      jetBoosterActive(false),
      jetBoosterCharges(0),
      jetBoosterBurstActive(false),
      jetBoosterBurstTimer(0.0f),
      spaceWasPressed(false),
      maxHealth(3),
      currentHealth(3),
      isAlive(true),
      isInvulnerable(false),
      invulnerableTimer(0.0f),
      isHitFlashing(false),
      hitFlashTimer(0.0f),
      flameVAO(0),
      flameVBO(0),
      flameAnimTimer(0.0f),
      flameShaderProgram(0) {
    
    setupMesh();
}

PlayerShip::~PlayerShip() {
    for (auto& submesh : subMeshes) {
        glDeleteVertexArrays(1, &submesh.VAO);
        glDeleteBuffers(1, &submesh.VBO);
        glDeleteBuffers(1, &submesh.EBO);
    }
    for (auto& material : materials) {
        glDeleteTextures(1, &material.diffuseTexture);
    }
    // Clean up flame resources
    if (flameVAO != 0) glDeleteVertexArrays(1, &flameVAO);
    if (flameVBO != 0) glDeleteBuffers(1, &flameVBO);
    if (flameShaderProgram != 0) glDeleteProgram(flameShaderProgram);
}

void PlayerShip::setupMesh() {
    const std::string basePath = "SpaceFighterExport/";
    const std::string objPath = basePath + "SmallSpaceFighter.obj";
    const std::string mtlPath = basePath + "SmallSpaceFighter.mtl";
    
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
                Material mat;
                mat.name = currentMat;
                mat.diffuseColor = glm::vec3(0.8f);
                mat.diffuseTexture = 0;
                mat.usesBakedLighting = true;  // Ship uses baked textures from Blender
                materialMap[currentMat] = materials.size();
                materials.push_back(mat);
            }
            else if (prefix == "map_Kd" && !currentMat.empty()) {
                std::string texFile;
                iss >> texFile;
                int idx = materialMap[currentMat];
                materials[idx].diffuseTexture = loadTexture((basePath + texFile).c_str());
            }
        }
        mtlFile.close();
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
    std::string currentObject = "";
    bool skipCurrentObject = false;
    
    std::ifstream objFile(objPath);
    if (!objFile.is_open()) {
        std::cerr << "Failed to load " << objPath << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(objFile, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "o") {
            iss >> currentObject;
            // Skip "Plane" and "Plane.001" objects (unwanted background cards from Blender)
            skipCurrentObject = (currentObject.find("Plane") == 0);
            if (skipCurrentObject) {
                std::cout << "Skipping object: " << currentObject << std::endl;
            }
        }
        else if (prefix == "v") {
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
            // Skip faces from unwanted objects
            if (skipCurrentObject) continue;
            
            std::vector<std::string> faceVerts;
            std::string vert;
            while (iss >> vert) {
                faceVerts.push_back(vert);
            }
            
            // Skip invalid faces
            if (faceVerts.size() < 3) continue;
            
            auto& group = groups[currentMaterial];
            group.material = currentMaterial;
            
            auto processFaceVertex = [&](const std::string& faceStr) -> unsigned int {
                unsigned int vIdx = 0, vtIdx = 0, vnIdx = 0;
                
                // Try v/vt/vn format first
                if (sscanf(faceStr.c_str(), "%u/%u/%u", &vIdx, &vtIdx, &vnIdx) != 3) {
                    // Try v//vn format (no texture coords)
                    if (sscanf(faceStr.c_str(), "%u//%u", &vIdx, &vnIdx) != 2) {
                        // Try v/vt format (no normals)
                        if (sscanf(faceStr.c_str(), "%u/%u", &vIdx, &vtIdx) != 2) {
                            // Try v format only
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
            
            // Triangulate face (fan triangulation for quads/n-gons)
            unsigned int firstIdx = processFaceVertex(faceVerts[0]);
            unsigned int prevIdx = processFaceVertex(faceVerts[1]);
            
            for (size_t i = 2; i < faceVerts.size(); i++) {
                unsigned int currIdx = processFaceVertex(faceVerts[i]);
                
                // Add triangle: first, prev, curr
                group.indices.push_back(firstIdx);
                group.indices.push_back(prevIdx);
                group.indices.push_back(currIdx);
                
                prevIdx = currIdx;
            }
        }
    }
    objFile.close();
    
    // Create submeshes
    for (auto& pair : groups) {
        SubMesh submesh;
        auto& group = pair.second;
        
        int matIdx = -1;
        if (materialMap.find(group.material) != materialMap.end()) {
            matIdx = materialMap[group.material];
        }
        submesh.materialIndex = matIdx;
        submesh.indexCount = group.indices.size();
        
        glGenVertexArrays(1, &submesh.VAO);
        glGenBuffers(1, &submesh.VBO);
        glGenBuffers(1, &submesh.EBO);
        
        glBindVertexArray(submesh.VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, submesh.VBO);
        glBufferData(GL_ARRAY_BUFFER, group.vertices.size() * sizeof(float), group.vertices.data(), GL_STATIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, submesh.EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, group.indices.size() * sizeof(unsigned int), group.indices.data(), GL_STATIC_DRAW);
        
        // Position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // UV
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
        
        subMeshes.push_back(submesh);
    }
    
    std::cout << "Loaded SmallSpaceFighter.obj: " << subMeshes.size() << " submeshes, " << materials.size() << " materials" << std::endl;
}

void PlayerShip::processInput(GLFWwindow* window, float deltaTime) {
    /*
     * ═══════════════════════════════════════════════════════════════
     *  LEVEL 1: SMOOTH ARCADE MOVEMENT WITH INERTIA
     * ═══════════════════════════════════════════════════════════════
     * 
     * Instead of instant velocity changes, we smoothly accelerate toward
     * a target velocity based on input. This gives the ship a nicer "arcade feel"
     * - not too floaty, not too grid-like.
     * 
     * Movement is frame-rate independent (uses deltaTime).
     * Ship moves only in X/Y plane - Z position is fixed (on-rails).
     */
    
    // Don't process input if ship is destroyed (Game Over state)
    if (!isAlive) {
        return;
    }
    
    // Step 1: Calculate target velocity based on WASD input
    glm::vec2 targetVel(0.0f);
    
    // W: Move UP (positive Y)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        targetVel.y += moveSpeed;
    }
    
    // S: Move DOWN (negative Y)
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        targetVel.y -= moveSpeed;
    }
    
    // A: Move LEFT (negative X)
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        targetVel.x -= moveSpeed;
    }
    
    // D: Move RIGHT (positive X)
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        targetVel.x += moveSpeed;
    }
    
    // Normalize diagonal input to prevent faster diagonal movement
    if (glm::length(targetVel) > moveSpeed) {
        targetVel = glm::normalize(targetVel) * moveSpeed;
    }
    
    // Space: Jet Booster burst activation (press to use a charge)
    bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spacePressed && !spaceWasPressed && jetBoosterCharges > 0 && !jetBoosterBurstActive) {
        // Consume a charge and start burst
        jetBoosterCharges--;
        jetBoosterBurstActive = true;
        jetBoosterBurstTimer = JET_BOOSTER_BURST_DURATION;
        std::cout << "[BOOST] Jet Booster activated! (" << jetBoosterCharges << " charges remaining)" << std::endl;
    }
    spaceWasPressed = spacePressed;
    
    // Apply boost speed if burst is active
    if (jetBoosterBurstActive) {
        targetVel *= 2.5f;  // 150% speed boost during burst
    }
    
    // Step 2: Smoothly interpolate current velocity toward target velocity
    // This creates smooth acceleration/deceleration for better arcade feel
    float smoothFactor = glm::clamp(moveAccel * deltaTime, 0.0f, 1.0f);
    velocity.x = glm::mix(velocity.x, targetVel.x, smoothFactor);
    velocity.y = glm::mix(velocity.y, targetVel.y, smoothFactor);
    velocity.z = 0.0f;  // Never move in Z (on-rails)
}

void PlayerShip::update(float deltaTime) {
    /*
     * ═══════════════════════════════════════════════════════════════
     *  LEVEL 1: ON-RAILS SHIP UPDATE
     * ═══════════════════════════════════════════════════════════════
     * 
     * Ship position updates in X/Y based on velocity (frame-rate independent).
     * Z position is ALWAYS fixed - world scrolls toward player instead.
     * Visual bank angle provides feedback for lateral movement (cosmetic).
     */
    
    // Update invulnerability timer (blinking indicates temporary invulnerability)
    if (isInvulnerable) {
        invulnerableTimer -= deltaTime;
        if (invulnerableTimer <= 0.0f) {
            isInvulnerable = false;
            invulnerableTimer = 0.0f;
        }
    }
    
    // Update hit flash timer (hit flash indicates damage just taken)
    if (isHitFlashing) {
        hitFlashTimer -= deltaTime;
        if (hitFlashTimer <= 0.0f) {
            isHitFlashing = false;
            hitFlashTimer = 0.0f;
        }
    }
    
    // Update jet booster burst timer
    if (jetBoosterBurstActive) {
        jetBoosterBurstTimer -= deltaTime;
        if (jetBoosterBurstTimer <= 0.0f) {
            jetBoosterBurstActive = false;
            jetBoosterBurstTimer = 0.0f;
            std::cout << "[BOOST] Jet Booster burst ended" << std::endl;
        }
    }
    
    // Don't process movement if ship is destroyed
    if (!isAlive) {
        // Gradually slow down when destroyed
        velocity *= 0.95f;
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
        position.z = fixedZ;
        clampPositionToBounds();
        return;
    }
    
    // Update position in X/Y plane only (movement is velocity * time)
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    position.z = fixedZ;  // Force Z to stay fixed (on-rails)
    
    // Clamp to Level 1 play area boundaries
    clampPositionToBounds();
    
    // Update visual bank angle based on horizontal velocity
    float targetBank = -velocity.x * (maxBankAngle / moveSpeed);  // Negative for intuitive tilt
    
    // Smoothly interpolate to target bank angle
    if (bankAngle < targetBank) {
        bankAngle += bankSpeed * deltaTime;
        if (bankAngle > targetBank) bankAngle = targetBank;
    } else if (bankAngle > targetBank) {
        bankAngle -= bankSpeed * deltaTime;
        if (bankAngle < targetBank) bankAngle = targetBank;
    }
    
    // Update flame animation timer
    flameAnimTimer += deltaTime;
}

void PlayerShip::clampPositionToBounds() {
    /*
     * LEVEL 1: Clamp ship to playable area boundaries
     * 
     * Keeps ship within [-50, 50] X and [-30, 30] Y.
     * Large area gives freedom of movement while preventing off-screen flying.
     */
    if (position.x < boundMinX) position.x = boundMinX;
    if (position.x > boundMaxX) position.x = boundMaxX;
    if (position.y < boundMinY) position.y = boundMinY;
    if (position.y > boundMaxY) position.y = boundMaxY;
}

void PlayerShip::setPosition(const glm::vec3& newPos) {
    position = newPos;
    position.z = fixedZ;  // Always enforce fixed Z
    clampPositionToBounds();
}

void PlayerShip::render(Shader& shader) {
    shader.use();
    
    // Set model matrix
    shader.setMat4("model", getModelMatrix());
    
    // Mark this as ship rendering for brightness boost
    shader.setInt("isShip", 1);
    
    // Set hit flash uniforms for red flash effect when damaged
    shader.setInt("isHitFlashing", isHitFlashing ? 1 : 0);
    if (isHitFlashing) {
        // Calculate flash intensity - pulsing effect
        float flashPulse = 0.5f + 0.5f * sin(hitFlashTimer * 15.0f);  // Fast pulsing
        shader.setFloat("hitFlashIntensity", flashPulse);
    } else {
        shader.setFloat("hitFlashIntensity", 0.0f);
    }
    
    // Draw all submeshes with their materials
    for (const auto& submesh : subMeshes) {
        // Set default values for uniforms
        shader.setInt("usesBakedLighting", 0);
        
        // Bind material texture
        if (submesh.materialIndex >= 0 && submesh.materialIndex < materials.size()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, materials[submesh.materialIndex].diffuseTexture);
            shader.setInt("texture1", 0);
            
            // Set baked lighting flag for this material
            shader.setInt("usesBakedLighting", materials[submesh.materialIndex].usesBakedLighting ? 1 : 0);
        } else {
            // Bind default white texture for submeshes without materials
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            shader.setInt("texture1", 0);
        }
        
        // Draw submesh
        glBindVertexArray(submesh.VAO);
        glDrawElements(GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
    
    // Reset isShip flag after ship rendering
    shader.setInt("isShip", 0);
    shader.setInt("isHitFlashing", 0);
    shader.setFloat("hitFlashIntensity", 0.0f);
}

void PlayerShip::renderEngineFlames(const glm::mat4& view, const glm::mat4& projection) {
    /*
     * ENGINE EXHAUST FLAMES
     * 
     * Renders animated flame cones behind the ship's engines.
     * - Normal: Blue fire color
     * - Boost active: Flickering red/orange
     */
    
    // Create flame shader if not yet created
    if (flameShaderProgram == 0) {
        const char* vertexShaderSource = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            out float vDepth;
            
            void main() {
                vec4 worldPos = model * vec4(aPos, 1.0);
                gl_Position = projection * view * worldPos;
                vDepth = aPos.z;  // Pass local Z for gradient
            }
        )";
        
        const char* fragmentShaderSource = R"(
            #version 330 core
            out vec4 FragColor;
            
            uniform vec3 flameColor;
            uniform float flickerIntensity;
            
            in float vDepth;
            
            void main() {
                // Fade out toward the tip of the flame
                float alpha = clamp(1.0 - (vDepth * 0.3), 0.2, 1.0);
                alpha *= flickerIntensity;
                
                // Brighter core, darker edges
                vec3 color = flameColor * (1.0 + flickerIntensity * 0.5);
                
                FragColor = vec4(color, alpha);
            }
        )";
        
        // Compile shaders
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        
        flameShaderProgram = glCreateProgram();
        glAttachShader(flameShaderProgram, vertexShader);
        glAttachShader(flameShaderProgram, fragmentShader);
        glLinkProgram(flameShaderProgram);
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
    // Create flame mesh if not yet created
    if (flameVAO == 0) {
        // Flame cone geometry - two engine exhausts
        // Each flame is a cone shape pointing backward (+Z direction)
        std::vector<float> vertices;
        
        // Engine positions (relative to ship center, behind the ship)
        float engineOffsetX[] = { -0.65f, 0.65f };  // Left and right engines (slightly wider)
        float engineY = 0.55f;   // Slightly higher to align with exhausts
        float engineZ = 1.0f;   // Behind the ship
        
        int segments = 8;
        float baseRadius = 0.4f;
        float flameLength = 2.5f;
        
        for (int e = 0; e < 2; e++) {
            float ex = engineOffsetX[e];
            
            // Create cone triangles
            for (int i = 0; i < segments; i++) {
                float angle1 = (float)i / segments * 2.0f * 3.14159f;
                float angle2 = (float)(i + 1) / segments * 2.0f * 3.14159f;
                
                // Base vertices
                float x1 = ex + cos(angle1) * baseRadius;
                float y1 = engineY + sin(angle1) * baseRadius;
                float z1 = engineZ;
                
                float x2 = ex + cos(angle2) * baseRadius;
                float y2 = engineY + sin(angle2) * baseRadius;
                float z2 = engineZ;
                
                // Tip vertex (extends backward)
                float tx = ex;
                float ty = engineY;
                float tz = engineZ + flameLength;
                
                // Triangle (base1, base2, tip)
                vertices.push_back(x1); vertices.push_back(y1); vertices.push_back(z1);
                vertices.push_back(x2); vertices.push_back(y2); vertices.push_back(z2);
                vertices.push_back(tx); vertices.push_back(ty); vertices.push_back(tz);
            }
        }
        
        glGenVertexArrays(1, &flameVAO);
        glGenBuffers(1, &flameVBO);
        
        glBindVertexArray(flameVAO);
        glBindBuffer(GL_ARRAY_BUFFER, flameVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        
        glBindVertexArray(0);
    }
    
    // Calculate flame color and intensity
    glm::vec3 flameColor;
    float flickerSpeed;
    
    if (jetBoosterBurstActive) {
        // Boost mode: Flickering red/orange
        float flicker = sin(flameAnimTimer * 25.0f) * 0.5f + 0.5f;  // Fast flicker
        float flicker2 = sin(flameAnimTimer * 37.0f) * 0.3f + 0.7f;  // Secondary flicker
        
        // Mix between red and orange
        flameColor = glm::mix(
            glm::vec3(1.0f, 0.2f, 0.0f),   // Red
            glm::vec3(1.0f, 0.6f, 0.0f),   // Orange
            flicker
        );
        flickerSpeed = 0.8f + flicker2 * 0.4f;
    } else {
        // Normal mode: Blue fire
        float flicker = sin(flameAnimTimer * 15.0f) * 0.2f + 0.8f;  // Gentle flicker
        
        flameColor = glm::vec3(0.2f, 0.5f, 1.0f);  // Blue
        flickerSpeed = 0.6f + flicker * 0.2f;
    }
    
    // Render flames
    glUseProgram(flameShaderProgram);
    
    // Set uniforms
    glm::mat4 model = getModelMatrix();
    GLint modelLoc = glGetUniformLocation(flameShaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    
    GLint viewLoc = glGetUniformLocation(flameShaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    
    GLint projLoc = glGetUniformLocation(flameShaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    
    GLint colorLoc = glGetUniformLocation(flameShaderProgram, "flameColor");
    glUniform3fv(colorLoc, 1, &flameColor[0]);
    
    GLint flickerLoc = glGetUniformLocation(flameShaderProgram, "flickerIntensity");
    glUniform1f(flickerLoc, flickerSpeed);
    
    // Enable blending for transparent flames
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive blending for glow effect
    glDepthMask(GL_FALSE);  // Don't write to depth buffer
    
    glBindVertexArray(flameVAO);
    glDrawArrays(GL_TRIANGLES, 0, 2 * 8 * 3);  // 2 engines * 8 segments * 3 vertices
    glBindVertexArray(0);
    
    // Restore state
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

glm::mat4 PlayerShip::getModelMatrix() const {
    /*
     * MODEL MATRIX FOR ON-RAILS SHIP
     * 
     * - No yaw or pitch rotation (always faces forward)
     * - Only visual bank (roll) for left/right movement
     * - Ship model's nose points along -Z (forward)
     */
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    
    // Apply visual bank angle (roll around Z axis) for cosmetic effect
    model = glm::rotate(model, glm::radians(bankAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    
    return model;
}

// ═══════════════════════════════════════════════════════════════
//  HEALTH & DAMAGE SYSTEM
// ═══════════════════════════════════════════════════════════════

void PlayerShip::takeDamage(int amount) {
    // Only take damage if alive and not currently invulnerable
    if (!isAlive || isInvulnerable) {
        return;
    }
    
    currentHealth -= amount;
    
    // Clamp health to 0
    if (currentHealth < 0) {
        currentHealth = 0;
    }
    
    // Check for death (Ship has 3 health; on 0, Level 1 enters Game Over state)
    if (currentHealth <= 0) {
        isAlive = false;
        std::cout << "[GAME OVER] Ship destroyed (0 health left)" << std::endl;
        return;
    }
    
    // Apply damage feedback
    // Trigger invulnerability window (prevents multiple HP lost in single collision cluster)
    isInvulnerable = true;
    invulnerableTimer = INVULNERABLE_DURATION;
    
    // Trigger hit flash visual feedback
    isHitFlashing = true;
    hitFlashTimer = HIT_FLASH_DURATION;
    
    // Apply knockback and shake - STRONG effect
    position.z += 4.0f;  // Stronger backward knockback
    // Add strong shake offset (random sideways jolt)
    float shakeX = ((rand() % 100) / 100.0f - 0.5f) * 4.0f;  // Strong X shake
    float shakeY = ((rand() % 100) / 100.0f - 0.5f) * 3.0f;  // Strong Y shake
    position.x += shakeX;
    position.y += shakeY;
    
    // Clamp so it doesn't pop through camera
    if (position.z > 0.0f) {
        position.z = 0.0f;
    }
    
    std::cout << "[DAMAGE] Ship hit! Health: " << currentHealth << " / " << maxHealth << std::endl;
}

void PlayerShip::heal(int amount) {
    if (!isAlive) return;
    
    currentHealth += amount;
    if (currentHealth > maxHealth) {
        currentHealth = maxHealth;
    }
    
    std::cout << "[HEAL] Health restored: " << currentHealth << " / " << maxHealth << std::endl;
}

void PlayerShip::resetHealth() {
    currentHealth = maxHealth;
    isAlive = true;
    isInvulnerable = false;
    invulnerableTimer = 0.0f;
    isHitFlashing = false;
    hitFlashTimer = 0.0f;
    
    std::cout << "[HEALTH] Ship health reset to " << currentHealth << " / " << maxHealth << std::endl;
}
