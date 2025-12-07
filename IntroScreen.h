#ifndef INTROSCREEN_H
#define INTROSCREEN_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <atomic>
#include <thread>
#include "SimpleTextRenderer.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mmsystem.h>
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "winmm.lib")
#endif

class IntroScreen {
public:
    IntroScreen();
    ~IntroScreen();
    
    // Initialize with window reference
    bool init(GLFWwindow* window, int screenWidth, int screenHeight);
    
    // Play the intro sequence - returns when complete or skipped
    // This will block and handle its own rendering
    void play();
    
    // Check if intro was skipped
    bool wasSkipped() const { return skipped; }
    
    // Cleanup resources
    void cleanup();

private:
    GLFWwindow* window;
    int screenWidth;
    int screenHeight;
    
    // Video playback state
    bool videoPlaying;
    bool skipped;
    bool freezeFrame;
    int currentVideoIndex;
    bool initialized;
    
    // Video file paths (wide strings for Windows APIs)
    std::wstring video1Path;
    std::wstring video2Path;
    std::wstring audioPath;
    
#ifdef _WIN32
    // Media Foundation for video
    IMFSourceReader* pReader;
    UINT32 videoWidth;
    UINT32 videoHeight;
    LONGLONG videoDuration;
    double frameRate;
    
    // Audio thread
    std::thread audioThread;
    std::atomic<bool> audioRunning;
#endif
    
    // OpenGL resources
    GLuint videoTexture;
    GLuint vao;
    GLuint vbo;
    GLuint shaderProgram;
    
    // Button resources
    GLuint buttonVao;
    GLuint buttonVbo;
    GLuint buttonShaderProgram;
    bool buttonHovered;
    bool playButtonClicked;
    bool controlsButtonHovered;
    bool controlsButtonClicked;
    bool showingControls;
    bool closeButtonHovered;
    SimpleTextRenderer textRenderer;
    
    // Frame buffer for video data
    unsigned char* frameBuffer;
    int frameBufferSize;
    
    // Timing
    double videoStartTime;
    double lastFrameTime;
    double targetFrameTime;
    
    // Private methods
    bool initOpenGLResources();
    bool initButtonResources();
    bool initMediaFoundation();
    void uninitMediaFoundation();
    bool openVideo(const std::wstring& path);
    void closeVideo();
    bool decodeNextFrame();
    void renderFrame();
    void renderFrameWithButton();
    void renderButton();
    void renderControlsButton();
    void renderControlsScreen();
    void renderCloseButton();
    void updateButtonHover();
    bool isMouseOverButton(double mouseX, double mouseY);
    bool isMouseOverControlsButton(double mouseX, double mouseY);
    bool isMouseOverCloseButton(double mouseX, double mouseY);
    void processInput();
    void processButtonInput();
    void startAudio();
    void stopAudio();
    
    // Shader sources
    static const char* vertexShaderSource;
    static const char* fragmentShaderSource;
    static const char* buttonVertexShaderSource;
    static const char* buttonFragmentShaderSource;
};

#endif
