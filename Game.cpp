#include "Game.h"
#include "Level1OuterDriftZone.h"
#include "Level2SolarDebrisPath.h"
#include "Texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Global pointer for callbacks
static Game* g_GameInstance = nullptr;

Game::Game() 
    : window(nullptr),
      screenWidth(1280),
      screenHeight(720),
      texturedShader(nullptr),
      playerShip(nullptr),
      cameraController(nullptr),
      levelManager(nullptr),
      introScreen(nullptr),
      deltaTime(0.0f),
      lastFrame(0.0f),
      firstMouse(true),
      rightMousePressed(false),
      rightMouseWasPressed(false),
      leftMousePressed(false),
      leftMouseWasPressed(false),
      lastMouseX(0.0),
      lastMouseY(0.0),
      lastShotTime(0.0f),
      shotCooldown(0.35f),
      nextSoundChannel(0) {  // 0.35 seconds between shots
    g_GameInstance = this;
}

Game::~Game() {
    cleanup();
}

bool Game::init() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // Create window
    window = glfwCreateWindow(screenWidth, screenHeight, "Starwave 3D", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    
    // Set input mode - we want to capture mouse when moving ship
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    // Load OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // Configure OpenGL
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, screenWidth, screenHeight);
    
    // Show a simple "Loading..." screen while assets load
    std::cout << "\n[Game] Loading game assets..." << std::endl;
    renderLoadingScreen("Loading game assets...");
    
    // Load shaders
    texturedShader = new Shader("shaders/textured.vert", "shaders/textured.frag");
    renderLoadingScreen("Loading shaders...");
    
    pbrShader = new Shader("shaders/pbr.vert", "shaders/pbr.frag");
    renderLoadingScreen("Loading PBR shaders...");
    
    // Create player ship
    playerShip = new PlayerShip();
    renderLoadingScreen("Loading player ship...");
    
    // Create camera controller (starts in third-person)
    cameraController = new CameraController();
    renderLoadingScreen("Initializing camera...");
    
    // Initialize loading screen text renderer
    loadingTextRenderer.init(screenWidth, screenHeight);
    
    // Setup loading screen HUD
    setupLoadingHUD();
    
    // Create and initialize level manager (this loads most assets)
    levelManager = new LevelManager();
    renderLoadingScreen("Loading levels...");
    
    // Initialize level manager (creates levels but doesn't load yet)
    levelManager->init(*this);
    renderLoadingScreen("Almost ready...");
    
    std::cout << "[Game] All assets loaded!" << std::endl;
    
    // NOW play the intro videos smoothly (all assets already loaded)
    introScreen = new IntroScreen();
    if (introScreen->init(window, screenWidth, screenHeight)) {
        introScreen->play();  // Videos play smoothly, no loading interference
        delete introScreen;
        introScreen = nullptr;
    } else {
        std::cout << "[Game] Intro screen initialization failed, continuing without intro" << std::endl;
        delete introScreen;
        introScreen = nullptr;
    }
    
    // Show loading screen after PLAY GAME is clicked
    renderLoadingScreen("Loading Level 1...");
    glfwPollEvents();  // Keep window responsive
    
    // Force reset to Level 1 after PLAY GAME is pressed
    levelManager->resetToFirstLevel(*this);
    
    std::cout << "\nStarwave 3D initialized successfully!" << std::endl;
    std::cout << "\n=== ON-RAILS ARCADE SHOOTER ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  W/S - Move ship UP/DOWN" << std::endl;
    std::cout << "  A/D - Move ship LEFT/RIGHT" << std::endl;
    std::cout << "  Space - Booster (future feature)" << std::endl;
    std::cout << "  Left Mouse Button - Shoot straight ahead" << std::endl;
    std::cout << "  Right Mouse Button - Toggle First/Third Person Camera" << std::endl;
    std::cout << "  Mouse Movement - NO EFFECT (disabled)" << std::endl;
    std::cout << "  ENTER - Complete level (temporary)" << std::endl;
    std::cout << "  F2 - Skip to Level 2 (debug)" << std::endl;
    std::cout << "\nGameplay: Move in X/Y plane, bullets always go forward!" << std::endl;
    std::cout << "=============================\n" << std::endl;
    
    // Initialize sound system for shooting
    initSoundSystem();
    
    return true;
}

void Game::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    // Don't process input if level manager or current level isn't ready
    if (!levelManager || !levelManager->getCurrentLevel()) {
        return;
    }
    
    // Check if controls are disabled (e.g., during win screen)
    if (levelManager->areControlsDisabled()) {
        return;  // Don't process any input during freeze
    }
    
    // DEBUG: Press N to print player position
    static bool nWasPressed = false;
    bool nPressed = (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS);
    if (nPressed && !nWasPressed) {
        glm::vec3 pos = playerShip->getPosition();
        std::cout << "[POSITION] Player at: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;
    }
    nWasPressed = nPressed;
    
    // DEBUG: Press F2 to skip directly to Level 2 for testing
    static bool f2WasPressed = false;
    bool f2Pressed = (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS);
    if (f2Pressed && !f2WasPressed) {
        std::cout << "\n[DEBUG] Skipping to Level 2..." << std::endl;
        levelManager->skipToLevel(1, *this);  // Index 1 = Level 2
    }
    f2WasPressed = f2Pressed;
    
    // Press 1 to teleport to Level 1 (only works if NOT in Level 1)
    static bool key1WasPressed = false;
    bool key1Pressed = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS);
    if (key1Pressed && !key1WasPressed) {
        if (levelManager->getCurrentLevelIndex() != 0) {
            std::cout << "\n[TELEPORT] Jumping to Level 1..." << std::endl;
            levelManager->skipToLevel(0, *this);
        } else {
            std::cout << "[TELEPORT] Already in Level 1!" << std::endl;
        }
    }
    key1WasPressed = key1Pressed;
    
    // Press 2 to collect all shards in Level 1 and activate portal (only works in Level 1)
    static bool key2WasPressed = false;
    bool key2Pressed = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS);
    if (key2Pressed && !key2WasPressed) {
        if (levelManager->getCurrentLevelIndex() == 0) {
            Level1OuterDriftZone* level1 = dynamic_cast<Level1OuterDriftZone*>(levelManager->getCurrentLevel());
            if (level1) {
                level1->collectAllShards();
            }
        } else {
            std::cout << "[CHEAT] Can only use in Level 1!" << std::endl;
        }
    }
    key2WasPressed = key2Pressed;
    
    // Press 3 to teleport to Level 2 (only works if NOT in Level 2)
    static bool key3WasPressed = false;
    bool key3Pressed = (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS);
    if (key3Pressed && !key3WasPressed) {
        if (levelManager->getCurrentLevelIndex() != 1) {
            std::cout << "\n[TELEPORT] Jumping to Level 2..." << std::endl;
            levelManager->skipToLevel(1, *this);
        } else {
            std::cout << "[TELEPORT] Already in Level 2!" << std::endl;
        }
    }
    key3WasPressed = key3Pressed;
    
    // Press 4 to collect all cells in Level 2 and activate portal (only works in Level 2)
    static bool key4WasPressed = false;
    bool key4Pressed = (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS);
    if (key4Pressed && !key4WasPressed) {
        if (levelManager->getCurrentLevelIndex() == 1) {
            Level2SolarDebrisPath* level2 = dynamic_cast<Level2SolarDebrisPath*>(levelManager->getCurrentLevel());
            if (level2) {
                level2->collectAllCells();
            }
        } else {
            std::cout << "[CHEAT] Can only use in Level 2!" << std::endl;
        }
    }
    key4WasPressed = key4Pressed;
    
    /*
     * ON-RAILS MODE: MOUSE MOVEMENT DISABLED
     * 
     * In the arcade on-rails style, the mouse does NOT control aiming or orientation.
     * Mouse movement code has been disabled - only mouse buttons are used:
     * - Left button: Shoot straight ahead
     * - Right button: Toggle camera mode
     */
    
    // Handle camera mode toggle (right mouse button)
    rightMousePressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    if (rightMousePressed && !rightMouseWasPressed) {
        cameraController->toggleMode();
        std::cout << "Camera: " << (cameraController->mode == CameraMode::FirstPerson ? "First-Person" : "Third-Person") << std::endl;
    }
    rightMouseWasPressed = rightMousePressed;
    
    // Handle shooting (left mouse button) with cooldown
    leftMousePressed = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    float currentTime = glfwGetTime();
    
    // Check if rapid fire power-up is active - bypasses normal cooldown
    bool rapidFireActive = levelManager->isRapidFireActive();
    float effectiveCooldown = rapidFireActive ? 0.05f : shotCooldown;  // Very fast fire rate when rapid fire active
    
    // Fire if button pressed AND cooldown elapsed (allow holding for auto-fire with delay)
    if (leftMousePressed && (currentTime - lastShotTime) >= effectiveCooldown) {
        /*
         * ON-RAILS SHOOTING - ALWAYS STRAIGHT AHEAD
         * 
         * Bullets always fire along the fixed forward direction (0, 0, -1).
         * No aiming - classic arcade style where you shoot what's in front of you.
         */
        glm::vec3 shipPos = playerShip->getPosition();
        glm::vec3 forwardDirection(0.0f, 0.0f, -1.0f);  // Always straight ahead
        
        // Tell the current level to fire a bullet
        levelManager->fireBulletInCurrentLevel(shipPos, forwardDirection);
        playShootSound();  // Play shooting sound effect
        lastShotTime = currentTime;
    }
    leftMouseWasPressed = leftMousePressed;
    
    // Let the player ship handle keyboard input
    playerShip->processInput(window, deltaTime);
}

void Game::update() {
    // Calculate delta time
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    // Update player ship
    playerShip->update(deltaTime);
    
    // Update camera controller with smooth follow (needs deltaTime for lerp)
    cameraController->update(playerShip->position, deltaTime);
    
    // Update level manager (handles level logic and switching)
    levelManager->update(deltaTime, *this);
}

void Game::render() {
    // Clear buffers
    glClearColor(0.05f, 0.05f, 0.15f, 1.0f); // Dark blue space background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Setup shader uniforms
    texturedShader->use();
    
    // Projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                           (float)screenWidth / (float)screenHeight, 
                                           0.1f, 1000.0f);
    texturedShader->setMat4("projection", projection);
    
    // View matrix from camera controller
    glm::mat4 view = cameraController->getViewMatrix();
    texturedShader->setMat4("view", view);
    
    // Basic lighting
    texturedShader->setVec3("lightColor", 1.0f, 1.0f, 1.0f);
    texturedShader->setVec3("lightPos", 0.0f, 100.0f, 0.0f);
    texturedShader->setVec3("viewPos", cameraController->position);
    
    // Render current level geometry
    levelManager->render(*this);
    
    // Render the player ship with hit flash effect
    if (playerShip->getIsHitFlashing()) {
        texturedShader->setInt("isHitFlash", 1);
        texturedShader->setVec3("hitFlashColor", glm::vec3(1.0f, 0.2f, 0.2f));  // Red flash
    } else {
        texturedShader->setInt("isHitFlash", 0);
    }
    playerShip->render(*texturedShader);
    texturedShader->setInt("isHitFlash", 0);  // Reset
    
    // Render engine flames (after ship, before swap)
    playerShip->renderEngineFlames(view, projection);
    
    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Game::run() {
    while (!glfwWindowShouldClose(window)) {
        processInput();
        update();
        render();
    }
}

void Game::returnToIntroScreen() {
    // Reset player ship to initial state
    if (playerShip) {
        playerShip->position = glm::vec3(0.0f, 0.0f, playerShip->fixedZ);
    }
    
    // Show intro screen with PLAY GAME button
    introScreen = new IntroScreen();
    if (introScreen->init(window, screenWidth, screenHeight)) {
        introScreen->play();
        delete introScreen;
        introScreen = nullptr;
    } else {
        std::cout << "[Game] Intro screen initialization failed" << std::endl;
        delete introScreen;
        introScreen = nullptr;
    }
    
    // Show loading screen after PLAY GAME is clicked
    renderLoadingScreen("Loading Level 1...");
    glfwPollEvents();  // Keep window responsive
    
    // Force reset to Level 1 by cleaning up current level and reloading
    levelManager->resetToFirstLevel(*this);
}

void Game::setupLoadingHUD() {
    // Create simple HUD shader for 2D quads
    const char* hudVertexShader = R"(
        #version 330 core
        layout(location = 0) in vec2 position;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(position, 0.0, 1.0);
        }
    )";
    
    const char* hudFragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec4 color;
        void main() {
            FragColor = color;
        }
    )";
    
    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &hudVertexShader, NULL);
    glCompileShader(vertexShader);
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &hudFragmentShader, NULL);
    glCompileShader(fragmentShader);
    
    loadingHudShader = glCreateProgram();
    glAttachShader(loadingHudShader, vertexShader);
    glAttachShader(loadingHudShader, fragmentShader);
    glLinkProgram(loadingHudShader);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Create VAO and VBO for quad
    glGenVertexArrays(1, &loadingHudVAO);
    glGenBuffers(1, &loadingHudVBO);
    
    glBindVertexArray(loadingHudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, loadingHudVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, NULL, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void Game::drawLoadingQuad(float x, float y, float width, float height, float r, float g, float b, float a) {
    if (loadingHudShader == 0) return;
    
    // Quad vertices
    float vertices[] = {
        x, y,
        x + width, y,
        x + width, y + height,
        x, y,
        x + width, y + height,
        x, y + height
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, loadingHudVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glUseProgram(loadingHudShader);
    
    // Get current framebuffer size for projection
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    
    glm::mat4 projection = glm::ortho(0.0f, (float)fbWidth, (float)fbHeight, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(loadingHudShader, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform4f(glGetUniformLocation(loadingHudShader, "color"), r, g, b, a);
    
    glBindVertexArray(loadingHudVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Game::renderLoadingScreen(const char* statusText) {
    // Dark space background
    glClearColor(0.01f, 0.01f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Get current window size
    int currentWidth, currentHeight;
    glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
    
    // Disable depth test, enable blending for HUD
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Animation values
    static float loadingProgress = 0.0f;
    loadingProgress += 0.015f;
    if (loadingProgress > 1.0f) loadingProgress = 0.0f;
    
    float pulse = 0.8f + 0.2f * sin(glfwGetTime() * 4.0f);
    
    float centerX = currentWidth / 2.0f;
    float centerY = currentHeight / 2.0f;
    
    // Scale all UI elements based on screen height
    float uiScale = currentHeight / 720.0f;
    
    // Draw corner brackets (L-shapes in each corner) - SCALED
    float bracketSize = 60.0f * uiScale;
    float bracketThick = 4.0f * uiScale;
    float frameOffset = 280.0f * uiScale;
    
    // Top-left corner bracket
    drawLoadingQuad(centerX - frameOffset, centerY - 130 * uiScale, bracketSize, bracketThick, 
                    0.2f * pulse, 0.6f * pulse, 1.0f * pulse, 0.8f);
    drawLoadingQuad(centerX - frameOffset, centerY - 130 * uiScale, bracketThick, bracketSize,
                    0.2f * pulse, 0.6f * pulse, 1.0f * pulse, 0.8f);
    
    // Top-right corner bracket
    drawLoadingQuad(centerX + frameOffset - bracketSize, centerY - 130 * uiScale, bracketSize, bracketThick,
                    0.2f * pulse, 0.6f * pulse, 1.0f * pulse, 0.8f);
    drawLoadingQuad(centerX + frameOffset - bracketThick, centerY - 130 * uiScale, bracketThick, bracketSize,
                    0.2f * pulse, 0.6f * pulse, 1.0f * pulse, 0.8f);
    
    // Bottom-left corner bracket
    drawLoadingQuad(centerX - frameOffset, centerY + 130 * uiScale - bracketThick, bracketSize, bracketThick,
                    0.2f * pulse, 0.6f * pulse, 1.0f * pulse, 0.8f);
    drawLoadingQuad(centerX - frameOffset, centerY + 130 * uiScale - bracketSize, bracketThick, bracketSize,
                    0.2f * pulse, 0.6f * pulse, 1.0f * pulse, 0.8f);
    
    // Bottom-right corner bracket
    drawLoadingQuad(centerX + frameOffset - bracketSize, centerY + 130 * uiScale - bracketThick, bracketSize, bracketThick,
                    0.2f * pulse, 0.6f * pulse, 1.0f * pulse, 0.8f);
    drawLoadingQuad(centerX + frameOffset - bracketThick, centerY + 130 * uiScale - bracketSize, bracketThick, bracketSize,
                    0.2f * pulse, 0.6f * pulse, 1.0f * pulse, 0.8f);
    
    // Loading bar - SCALED
    float barWidth = 500.0f * uiScale;
    float barHeight = 25.0f * uiScale;
    float barY = centerY + 60.0f * uiScale;
    
    // Bar background (dark)
    drawLoadingQuad(centerX - barWidth/2, barY, barWidth, barHeight, 0.05f, 0.05f, 0.15f, 0.9f);
    
    // Bar fill (animated)
    float fillWidth = barWidth * loadingProgress;
    drawLoadingQuad(centerX - barWidth/2, barY, fillWidth, barHeight, 
                    0.1f * pulse, 0.5f * pulse, 1.0f * pulse, 0.9f);
    
    // Bar border - SCALED
    float borderThick = 2.0f * uiScale;
    drawLoadingQuad(centerX - barWidth/2, barY, barWidth, borderThick, 
                    0.3f * pulse, 0.7f * pulse, 1.0f * pulse, 1.0f);
    drawLoadingQuad(centerX - barWidth/2, barY + barHeight - borderThick, barWidth, borderThick,
                    0.3f * pulse, 0.7f * pulse, 1.0f * pulse, 1.0f);
    drawLoadingQuad(centerX - barWidth/2, barY, borderThick, barHeight,
                    0.3f * pulse, 0.7f * pulse, 1.0f * pulse, 1.0f);
    drawLoadingQuad(centerX + barWidth/2 - borderThick, barY, borderThick, barHeight,
                    0.3f * pulse, 0.7f * pulse, 1.0f * pulse, 1.0f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    
    // Text rendering
    loadingTextRenderer.updateScreenSize(currentWidth, currentHeight);
    
    std::string displayText = std::string(statusText);
    int dotCount = (int)(glfwGetTime() * 2.0) % 4;
    for (int i = 0; i < dotCount; ++i) {
        displayText += ".";
    }
    
    // Scale text based on screen height for consistent size across resolutions
    float textScale = (currentHeight / 720.0f) * 2.5f;  // Scale relative to 720p
    
    // Measure actual text width for precise centering
    float charWidth = 8.0f * textScale;  // Base character width from shader
    float textWidth = displayText.length() * charWidth;
    
    // DEAD CENTER horizontally and slightly above vertical center
    float textX = (currentWidth - textWidth) / 2.0f;
    float textY = (currentHeight / 2.0f) - (30.0f * (currentHeight / 720.0f));
    
    glm::vec3 textColor(0.7f + 0.3f * pulse, 0.9f + 0.1f * pulse, 1.0f);
    loadingTextRenderer.renderTextScaled(displayText, textX, textY, textColor, textScale);
    
    // Subtitle scaled proportionally
    std::string subtitle = "INITIALIZING SYSTEMS";
    float subtitleScale = (currentHeight / 720.0f) * 1.0f;
    float subtitleWidth = subtitle.length() * 8.0f * subtitleScale;
    
    // DEAD CENTER horizontally
    float subtitleX = (currentWidth - subtitleWidth) / 2.0f;
    float subtitleY = textY + (40.0f * (currentHeight / 720.0f));
    
    glm::vec3 subtitleColor(0.4f * pulse, 0.6f * pulse, 0.8f * pulse);
    loadingTextRenderer.renderTextScaled(subtitle, subtitleX, subtitleY, subtitleColor, subtitleScale);
    
    glfwSwapBuffers(window);
    glfwPollEvents();
    
    std::cout << "[Loading] " << statusText << std::endl;
}

void Game::cleanup() {
    cleanupSoundSystem();
    
    delete introScreen;
    delete levelManager;
    delete playerShip;
    delete cameraController;
    delete texturedShader;
    delete pbrShader;
    
    glfwTerminate();
}

void Game::initSoundSystem() {
#ifdef _WIN32
    std::cout << "[Sound] Sound system ready (using WAV for low latency)" << std::endl;
#endif
}

void Game::cleanupSoundSystem() {
#ifdef _WIN32
    // Stop any playing sound
    PlaySoundW(NULL, NULL, 0);
#endif
}

void Game::playShootSound() {
#ifdef _WIN32
    // Use PlaySound with WAV file for instant playback
    // SND_ASYNC = non-blocking, SND_NOSTOP = don't interrupt if already playing (allows overlap feeling)
    PlaySoundW(L"shooting/shoot.wav", NULL, SND_FILENAME | SND_ASYNC);
#endif
}

void Game::playCollectSound() {
#ifdef _WIN32
    PlaySoundW(L"Collecting_collectibles/collect.wav", NULL, SND_FILENAME | SND_ASYNC);
#endif
}

void Game::playPowerupSound() {
#ifdef _WIN32
    PlaySoundW(L"Collecting_powerups/powerup.wav", NULL, SND_FILENAME | SND_ASYNC);
#endif
}

void Game::playCrashSound() {
#ifdef _WIN32
    PlaySoundW(L"Crashing/crash.wav", NULL, SND_FILENAME | SND_ASYNC);
#endif
}

void Game::playGameOverSound() {
#ifdef _WIN32
    PlaySoundW(L"GameOver/gameover.wav", NULL, SND_FILENAME | SND_ASYNC);
#endif
}

void Game::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void Game::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // This callback can be used for additional mouse handling if needed
    // Currently, we handle right-click in processInput()
}
