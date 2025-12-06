#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "PlayerShip.h"
#include "CameraController.h"
#include "LevelManager.h"
#include "Shader.h"
#include "IntroScreen.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

class Game {
public:
    Game();
    ~Game();
    
    bool init();
    void run();
    void cleanup();
    
    // Accessors for levels to use
    GLFWwindow* getWindow() const { return window; }
    Shader* getTexturedShader() const { return texturedShader; }
    Shader* getPBRShader() const { return pbrShader; }
    PlayerShip* getPlayerShip() const { return playerShip; }
    CameraController* getCameraController() const { return cameraController; }
    int getScreenWidth() const { return screenWidth; }
    int getScreenHeight() const { return screenHeight; }
    
    // Sound effects (public for levels to use)
    void playShootSound();      // Play shooting sound effect
    void playCollectSound();    // Play collectible pickup sound
    void playPowerupSound();    // Play powerup pickup sound  
    void playCrashSound();      // Play crash/impact sound
    void playGameOverSound();   // Play game over sound
    
private:
    GLFWwindow* window;
    int screenWidth;
    int screenHeight;
    
    Shader* texturedShader;
    Shader* pbrShader;
    PlayerShip* playerShip;
    CameraController* cameraController;
    LevelManager* levelManager;
    IntroScreen* introScreen;
    
    float deltaTime;
    float lastFrame;
    
    // Mouse input state
    bool firstMouse;
    bool rightMousePressed;
    bool rightMouseWasPressed;
    bool leftMousePressed;
    bool leftMouseWasPressed;
    double lastMouseX;
    double lastMouseY;
    
    // Shot cooldown
    float lastShotTime;
    float shotCooldown;  // Time between shots
    
    // Sound system
    int nextSoundChannel;  // Rotating channel for overlapping sounds
    static const int MAX_SOUND_CHANNELS = 8;
    
    void processInput();
    void update();
    void render();
    void renderLoadingScreen(const char* statusText);  // Simple loading screen
    void initSoundSystem();
    void cleanupSoundSystem();
    
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};

#endif
