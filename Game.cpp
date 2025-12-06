#include "Game.h"
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
    
    // Create and initialize level manager (this loads most assets)
    levelManager = new LevelManager();
    renderLoadingScreen("Loading levels...");
    
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
    
    // Check if controls are disabled (e.g., during win screen)
    if (levelManager && levelManager->areControlsDisabled()) {
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

void Game::renderLoadingScreen(const char* statusText) {
    // Simple black screen with console output - keeps window responsive
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);  // Very dark blue
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
