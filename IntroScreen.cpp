#include "IntroScreen.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
#include <mferror.h>
#include <propvarutil.h>
#pragma comment(lib, "propsys.lib")
#endif

// Shader sources for fullscreen quad
const char* IntroScreen::vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* IntroScreen::fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D videoTexture;
void main() {
    FragColor = texture(videoTexture, TexCoord);
}
)";

// Button shaders
const char* IntroScreen::buttonVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* IntroScreen::buttonFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 buttonColor;
uniform vec2 buttonCenter;
uniform vec2 buttonSize;
uniform float cornerRadius;
uniform int renderText;
uniform vec2 screenSize;

// Simple "PLAY GAME" text rendering using distance fields
float sdBox(vec2 p, vec2 b) {
    vec2 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdRoundedBox(vec2 p, vec2 b, float r) {
    vec2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

// Character rendering for "PLAY GAME" - simplified blocky font
float renderChar(vec2 uv, int c) {
    float d = 1.0;
    float w = 0.15;  // stroke width
    
    // P
    if (c == 0) {
        d = min(d, sdBox(uv - vec2(-0.3, 0.0), vec2(w, 0.5)));  // vertical
        d = min(d, sdBox(uv - vec2(-0.1, 0.35), vec2(0.2, w)));  // top horizontal
        d = min(d, sdBox(uv - vec2(-0.1, 0.0), vec2(0.2, w)));   // middle horizontal
        d = min(d, sdBox(uv - vec2(0.1, 0.175), vec2(w, 0.175))); // right vertical
    }
    // L
    else if (c == 1) {
        d = min(d, sdBox(uv - vec2(-0.3, 0.0), vec2(w, 0.5)));  // vertical
        d = min(d, sdBox(uv - vec2(0.0, -0.35), vec2(0.3, w)));  // bottom horizontal
    }
    // A
    else if (c == 2) {
        d = min(d, sdBox(uv - vec2(-0.2, 0.0), vec2(w, 0.5)));  // left vertical
        d = min(d, sdBox(uv - vec2(0.2, 0.0), vec2(w, 0.5)));   // right vertical
        d = min(d, sdBox(uv - vec2(0.0, 0.35), vec2(0.2, w)));  // top horizontal
        d = min(d, sdBox(uv - vec2(0.0, 0.0), vec2(0.2, w)));   // middle horizontal
    }
    // Y
    else if (c == 3) {
        d = min(d, sdBox(uv - vec2(-0.2, 0.25), vec2(w, 0.25))); // top left
        d = min(d, sdBox(uv - vec2(0.2, 0.25), vec2(w, 0.25)));  // top right
        d = min(d, sdBox(uv - vec2(0.0, -0.175), vec2(w, 0.325))); // bottom center
    }
    // G
    else if (c == 4) {
        d = min(d, sdBox(uv - vec2(-0.3, 0.0), vec2(w, 0.5)));  // left vertical
        d = min(d, sdBox(uv - vec2(0.0, 0.35), vec2(0.3, w)));  // top horizontal
        d = min(d, sdBox(uv - vec2(0.0, -0.35), vec2(0.3, w))); // bottom horizontal
        d = min(d, sdBox(uv - vec2(0.15, -0.175), vec2(w, 0.175))); // right bottom vertical
        d = min(d, sdBox(uv - vec2(0.0, 0.0), vec2(0.15, w)));  // middle horizontal
    }
    // M
    else if (c == 5) {
        d = min(d, sdBox(uv - vec2(-0.3, 0.0), vec2(w, 0.5)));  // left vertical
        d = min(d, sdBox(uv - vec2(0.3, 0.0), vec2(w, 0.5)));   // right vertical
        d = min(d, sdBox(uv - vec2(-0.15, 0.2), vec2(w, 0.15))); // left diagonal approx
        d = min(d, sdBox(uv - vec2(0.15, 0.2), vec2(w, 0.15)));  // right diagonal approx
        d = min(d, sdBox(uv - vec2(0.0, 0.35), vec2(0.15, w)));  // top connect
    }
    // E
    else if (c == 6) {
        d = min(d, sdBox(uv - vec2(-0.3, 0.0), vec2(w, 0.5)));  // vertical
        d = min(d, sdBox(uv - vec2(0.0, 0.35), vec2(0.3, w)));  // top horizontal
        d = min(d, sdBox(uv - vec2(0.0, 0.0), vec2(0.25, w)));  // middle horizontal
        d = min(d, sdBox(uv - vec2(0.0, -0.35), vec2(0.3, w))); // bottom horizontal
    }
    // Space
    else if (c == 7) {
        d = 1.0;
    }
    
    return d;
}

void main() {
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 center = vec2(screenSize.x * 0.5, screenSize.y * 0.5);
    
    if (renderText == 1) {
        // Render text "PLAY GAME" - centered on screen
        vec2 textPos = (fragCoord - center) / (screenSize.y * 0.05);
        
        float textAlpha = 0.0;
        
        // Character positions for "PLAY GAME"
        // P L A Y   G A M E
        int chars[9] = int[9](0, 1, 2, 3, 7, 4, 2, 5, 6);
        float startX = -4.0;
        
        for (int i = 0; i < 9; i++) {
            vec2 charUV = textPos - vec2(startX + float(i) * 1.0, 0.0);
            float d = renderChar(charUV, chars[i]);
            if (d < 0.0) {
                textAlpha = 1.0;
            }
        }
        
        FragColor = vec4(1.0, 1.0, 1.0, textAlpha);
    } else {
        // Render button background - centered on screen
        vec2 p = fragCoord - center;
        float d = sdRoundedBox(p, buttonSize * 0.5, cornerRadius);
        
        if (d < 0.0) {
            FragColor = buttonColor;
        } else if (d < 3.0) {
            // Border
            FragColor = vec4(1.0, 1.0, 1.0, 0.9);
        } else {
            discard;
        }
    }
}
)";

IntroScreen::IntroScreen() 
    : window(nullptr),
      screenWidth(1280),
      screenHeight(720),
      videoPlaying(false),
      skipped(false),
      freezeFrame(false),
      currentVideoIndex(0),
      initialized(false),
#ifdef _WIN32
      pReader(nullptr),
      videoWidth(0),
      videoHeight(0),
      videoDuration(0),
      frameRate(30.0),
      audioRunning(false),
#endif
      videoTexture(0),
      vao(0),
      vbo(0),
      shaderProgram(0),
      frameBuffer(nullptr),
      frameBufferSize(0),
      videoStartTime(0.0),
      lastFrameTime(0.0),
      targetFrameTime(1.0 / 30.0),
      buttonVao(0),
      buttonVbo(0),
      buttonShaderProgram(0),
      buttonHovered(false),
      playButtonClicked(false) {
    
    // Set paths relative to executable (wide strings for Windows)
    video1Path = L"Intro/209588.mp4";
    video2Path = L"Intro/218486.mp4";
    audioPath = L"Intro/deep-church-choir-337100.mp3";
}

IntroScreen::~IntroScreen() {
    cleanup();
}

bool IntroScreen::init(GLFWwindow* win, int width, int height) {
    window = win;
    screenWidth = width;
    screenHeight = height;
    
    if (!initOpenGLResources()) {
        std::cerr << "[Intro] Failed to initialize OpenGL resources" << std::endl;
        return false;
    }
    
    if (!initButtonResources()) {
        std::cerr << "[Intro] Failed to initialize button resources" << std::endl;
        return false;
    }
    
#ifdef _WIN32
    if (!initMediaFoundation()) {
        std::cerr << "[Intro] Failed to initialize Media Foundation" << std::endl;
        return false;
    }
#endif
    
    initialized = true;
    std::cout << "[Intro] Intro screen initialized successfully" << std::endl;
    return true;
}

bool IntroScreen::initOpenGLResources() {
    // Create fullscreen quad vertices (position + texcoord)
    // Flipped Y texcoords because video is typically top-down
    float vertices[] = {
        // positions   // texcoords
        -1.0f,  1.0f,  0.0f, 0.0f,  // top left
         1.0f,  1.0f,  1.0f, 0.0f,  // top right
         1.0f, -1.0f,  1.0f, 1.0f,  // bottom right
        -1.0f,  1.0f,  0.0f, 0.0f,  // top left
         1.0f, -1.0f,  1.0f, 1.0f,  // bottom right
        -1.0f, -1.0f,  0.0f, 1.0f   // bottom left
    };
    
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "[Intro] Vertex shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "[Intro] Fragment shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "[Intro] Shader program linking failed: " << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Create video texture
    glGenTextures(1, &videoTexture);
    glBindTexture(GL_TEXTURE_2D, videoTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    return true;
}

bool IntroScreen::initButtonResources() {
    // Create fullscreen quad for button rendering
    float buttonVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    
    glGenVertexArrays(1, &buttonVao);
    glGenBuffers(1, &buttonVbo);
    
    glBindVertexArray(buttonVao);
    glBindBuffer(GL_ARRAY_BUFFER, buttonVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buttonVertices), buttonVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(0);
    
    // Compile button shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &buttonVertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "[Intro] Button vertex shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &buttonFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "[Intro] Button fragment shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    
    buttonShaderProgram = glCreateProgram();
    glAttachShader(buttonShaderProgram, vertexShader);
    glAttachShader(buttonShaderProgram, fragmentShader);
    glLinkProgram(buttonShaderProgram);
    
    glGetProgramiv(buttonShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(buttonShaderProgram, 512, NULL, infoLog);
        std::cerr << "[Intro] Button shader program linking failed: " << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    std::cout << "[Intro] Button resources initialized" << std::endl;
    return true;
}

#ifdef _WIN32

bool IntroScreen::initMediaFoundation() {
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        std::cerr << "[Intro] MFStartup failed" << std::endl;
        return false;
    }
    return true;
}

void IntroScreen::uninitMediaFoundation() {
    MFShutdown();
}

bool IntroScreen::openVideo(const std::wstring& path) {
    closeVideo();
    
    // Create source reader
    IMFAttributes* pAttributes = nullptr;
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) {
        std::cerr << "[Intro] Failed to create attributes" << std::endl;
        return false;
    }
    
    // Configure to decode video
    hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    
    hr = MFCreateSourceReaderFromURL(path.c_str(), pAttributes, &pReader);
    pAttributes->Release();
    
    if (FAILED(hr)) {
        std::wcerr << L"[Intro] Failed to open video: " << path << std::endl;
        return false;
    }
    
    // Configure output format to RGB32
    IMFMediaType* pType = nullptr;
    hr = MFCreateMediaType(&pType);
    if (SUCCEEDED(hr)) {
        hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    }
    if (SUCCEEDED(hr)) {
        hr = pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    }
    if (SUCCEEDED(hr)) {
        hr = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pType);
    }
    pType->Release();
    
    if (FAILED(hr)) {
        std::cerr << "[Intro] Failed to set output format" << std::endl;
        closeVideo();
        return false;
    }
    
    // Get the actual output format
    IMFMediaType* pOutputType = nullptr;
    hr = pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pOutputType);
    if (SUCCEEDED(hr)) {
        // Get video dimensions
        MFGetAttributeSize(pOutputType, MF_MT_FRAME_SIZE, &videoWidth, &videoHeight);
        
        // Get frame rate
        UINT32 numerator, denominator;
        MFGetAttributeRatio(pOutputType, MF_MT_FRAME_RATE, &numerator, &denominator);
        if (denominator > 0) {
            frameRate = (double)numerator / (double)denominator;
            targetFrameTime = 1.0 / frameRate;
        }
        
        pOutputType->Release();
    }
    
    // Get duration
    PROPVARIANT var;
    PropVariantInit(&var);
    hr = pReader->GetPresentationAttribute((DWORD)MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
    if (SUCCEEDED(hr)) {
        videoDuration = var.hVal.QuadPart;
        PropVariantClear(&var);
    }
    
    // Allocate frame buffer (BGRA format - 4 bytes per pixel)
    frameBufferSize = videoWidth * videoHeight * 4;
    if (frameBuffer) {
        delete[] frameBuffer;
    }
    frameBuffer = new unsigned char[frameBufferSize];
    
    std::wcout << L"[Intro] Opened video: " << path 
               << L" (" << videoWidth << L"x" << videoHeight 
               << L", " << frameRate << L" fps)" << std::endl;
    
    return true;
}

void IntroScreen::closeVideo() {
    if (pReader) {
        pReader->Release();
        pReader = nullptr;
    }
    videoWidth = 0;
    videoHeight = 0;
}

bool IntroScreen::decodeNextFrame() {
    if (!pReader) return false;
    
    DWORD streamIndex, flags;
    LONGLONG timestamp;
    IMFSample* pSample = nullptr;
    
    HRESULT hr = pReader->ReadSample(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        &streamIndex,
        &flags,
        &timestamp,
        &pSample
    );
    
    if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
        if (pSample) pSample->Release();
        return false;  // End of video or error
    }
    
    if (pSample) {
        IMFMediaBuffer* pBuffer = nullptr;
        hr = pSample->ConvertToContiguousBuffer(&pBuffer);
        
        if (SUCCEEDED(hr)) {
            BYTE* pData = nullptr;
            DWORD maxLength, currentLength;
            
            hr = pBuffer->Lock(&pData, &maxLength, &currentLength);
            if (SUCCEEDED(hr)) {
                // Copy and convert BGRA to RGBA for OpenGL
                for (UINT32 i = 0; i < videoWidth * videoHeight; i++) {
                    frameBuffer[i * 4 + 0] = pData[i * 4 + 2];  // R <- B
                    frameBuffer[i * 4 + 1] = pData[i * 4 + 1];  // G <- G
                    frameBuffer[i * 4 + 2] = pData[i * 4 + 0];  // B <- R
                    frameBuffer[i * 4 + 3] = pData[i * 4 + 3];  // A <- A
                }
                
                // Upload to OpenGL texture (flip Y because video is top-down)
                glBindTexture(GL_TEXTURE_2D, videoTexture);
                
                // Flip the image vertically
                unsigned char* flippedBuffer = new unsigned char[frameBufferSize];
                int rowSize = videoWidth * 4;
                for (UINT32 y = 0; y < videoHeight; y++) {
                    memcpy(flippedBuffer + (videoHeight - 1 - y) * rowSize,
                           frameBuffer + y * rowSize,
                           rowSize);
                }
                
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, videoWidth, videoHeight,
                            0, GL_RGBA, GL_UNSIGNED_BYTE, flippedBuffer);
                
                delete[] flippedBuffer;
                
                pBuffer->Unlock();
            }
            pBuffer->Release();
        }
        pSample->Release();
        return true;
    }
    
    return false;
}

void IntroScreen::startAudio() {
    audioRunning = true;
    
    // Convert wide string to narrow for PlaySound
    std::wstring fullPath = audioPath;
    
    audioThread = std::thread([this, fullPath]() {
        // Use PlaySound for simple MP3 playback via MCI
        std::wstring mciCommand = L"open \"" + fullPath + L"\" type mpegvideo alias intromusic";
        mciSendStringW(mciCommand.c_str(), NULL, 0, NULL);
        mciSendStringW(L"play intromusic", NULL, 0, NULL);
        
        std::cout << "[Intro] Audio playback started" << std::endl;
        
        // Keep thread alive while audio should run
        while (audioRunning) {
            Sleep(100);
        }
        
        mciSendStringW(L"stop intromusic", NULL, 0, NULL);
        mciSendStringW(L"close intromusic", NULL, 0, NULL);
    });
}

void IntroScreen::stopAudio() {
    audioRunning = false;
    if (audioThread.joinable()) {
        audioThread.join();
    }
}

#else
// Stub implementations for non-Windows
bool IntroScreen::initMediaFoundation() { return false; }
void IntroScreen::uninitMediaFoundation() {}
bool IntroScreen::openVideo(const std::wstring& path) { return false; }
void IntroScreen::closeVideo() {}
bool IntroScreen::decodeNextFrame() { return false; }
void IntroScreen::startAudio() {}
void IntroScreen::stopAudio() {}
#endif

void IntroScreen::renderFrame() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_DEPTH_TEST);
    
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, videoTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "videoTexture"), 0);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glEnable(GL_DEPTH_TEST);
    
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void IntroScreen::renderFrameWithButton() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_DEPTH_TEST);
    
    // First render the video frame
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, videoTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "videoTexture"), 0);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Then render button on top
    renderButton();
    
    glEnable(GL_DEPTH_TEST);
    
    glfwSwapBuffers(window);
}

void IntroScreen::renderButton() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(buttonShaderProgram);
    glBindVertexArray(buttonVao);
    
    // Button dimensions - match isMouseOverButton
    float buttonWidth = 350.0f;
    float buttonHeight = 80.0f;
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    
    // Set uniforms for button background
    glUniform4f(glGetUniformLocation(buttonShaderProgram, "buttonColor"), 
                buttonHovered ? 0.3f : 0.15f,   // R - lighter when hovered
                buttonHovered ? 0.6f : 0.4f,    // G
                buttonHovered ? 0.9f : 0.7f,    // B - blue tint
                0.85f);                          // A
    glUniform2f(glGetUniformLocation(buttonShaderProgram, "buttonCenter"), centerX, screenHeight - centerY);  // Flip Y for OpenGL
    glUniform2f(glGetUniformLocation(buttonShaderProgram, "buttonSize"), buttonWidth, buttonHeight);
    glUniform1f(glGetUniformLocation(buttonShaderProgram, "cornerRadius"), 15.0f);
    glUniform1i(glGetUniformLocation(buttonShaderProgram, "renderText"), 0);
    glUniform2f(glGetUniformLocation(buttonShaderProgram, "screenSize"), (float)screenWidth, (float)screenHeight);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Render text
    glUniform1i(glGetUniformLocation(buttonShaderProgram, "renderText"), 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisable(GL_BLEND);
}

void IntroScreen::updateButtonHover() {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    buttonHovered = isMouseOverButton(mouseX, mouseY);
}

bool IntroScreen::isMouseOverButton(double mouseX, double mouseY) {
    float buttonWidth = 350.0f;
    float buttonHeight = 80.0f;
    float centerX = screenWidth / 2.0f;
    float centerY = screenHeight / 2.0f;
    
    // Button bounds (GLFW has Y=0 at top, so this works correctly)
    float left = centerX - buttonWidth / 2.0f;
    float right = centerX + buttonWidth / 2.0f;
    float top = centerY - buttonHeight / 2.0f;
    float bottom = centerY + buttonHeight / 2.0f;
    
    return mouseX >= left && mouseX <= right && mouseY >= top && mouseY <= bottom;
}

void IntroScreen::processButtonInput() {
    static bool wasPressed = false;
    bool isPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    // Detect click (press while over button)
    if (isPressed && buttonHovered) {
        playButtonClicked = true;
    }
    
    wasPressed = isPressed;
}

void IntroScreen::processInput() {
    // Skip intro on Space, Enter, or Escape
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
        glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        skipped = true;
        videoPlaying = false;
    }
}

void IntroScreen::play() {
    if (!initialized) {
        std::cerr << "[Intro] IntroScreen not initialized!" << std::endl;
        return;
    }
    
    std::cout << "\n=== INTRO SEQUENCE STARTING ===" << std::endl;
    std::cout << "Press SPACE, ENTER, ESC, or CLICK to skip" << std::endl;
    
#ifdef _WIN32
    // Start background music
    startAudio();
    
    // Play first video
    currentVideoIndex = 1;
    if (!openVideo(video1Path)) {
        std::cerr << "[Intro] Failed to open first video, skipping intro" << std::endl;
        stopAudio();
        return;
    }
    
    videoPlaying = true;
    videoStartTime = glfwGetTime();
    lastFrameTime = videoStartTime;
    
    // Main intro loop
    while (videoPlaying && !glfwWindowShouldClose(window)) {
        processInput();
        
        if (!videoPlaying) break;
        
        double currentTime = glfwGetTime();
        double elapsed = currentTime - lastFrameTime;
        
        // Frame timing for smooth playback
        if (elapsed >= targetFrameTime) {
            lastFrameTime = currentTime;
            
            if (!freezeFrame) {
                if (!decodeNextFrame()) {
                    // Video ended
                    if (currentVideoIndex == 1) {
                        // Switch to second video
                        currentVideoIndex = 2;
                        if (!openVideo(video2Path)) {
                            std::cerr << "[Intro] Failed to open second video" << std::endl;
                            break;
                        }
                        std::cout << "[Intro] Switching to second video" << std::endl;
                    } else {
                        // Second video ended - freeze on last frame with Play Game button
                        std::cout << "[Intro] Second video ended, showing Play Game button" << std::endl;
                        freezeFrame = true;
                        playButtonClicked = false;
                        
                        // Keep frozen until Play Game button is clicked
                        double freezeStartTime = glfwGetTime();
                        double maxFreezeTime = 120.0;  // Max 2 minutes freeze (longer since waiting for button)
                        
                        while (!glfwWindowShouldClose(window)) {
                            glfwPollEvents();
                            
                            // Update button hover state
                            updateButtonHover();
                            
                            // Process button input
                            processButtonInput();
                            
                            // Exit only when button is clicked
                            if (playButtonClicked) {
                                std::cout << "[Intro] Play Game button clicked!" << std::endl;
                                videoPlaying = false;
                                break;
                            }
                            
                            // Check timeout (safety)
                            if (glfwGetTime() - freezeStartTime > maxFreezeTime) {
                                std::cout << "[Intro] Freeze timeout, ending intro" << std::endl;
                                videoPlaying = false;
                                break;
                            }
                            
                            // Render frame with button overlay
                            renderFrameWithButton();
                            
                            // Small wait to prevent CPU spinning
                            glfwWaitEventsTimeout(0.016);  // ~60fps
                        }
                    }
                }
            }
            
            renderFrame();
        }
    }
    
    // Cleanup
    stopAudio();
    closeVideo();
#else
    std::cout << "[Intro] Video playback not supported on this platform" << std::endl;
#endif
    
    if (skipped) {
        std::cout << "[Intro] Intro skipped by user" << std::endl;
    } else {
        std::cout << "[Intro] Intro sequence complete" << std::endl;
    }
    std::cout << "=== STARTING GAME ===" << std::endl;
}

void IntroScreen::cleanup() {
#ifdef _WIN32
    stopAudio();
    closeVideo();
    uninitMediaFoundation();
#endif
    
    if (frameBuffer) {
        delete[] frameBuffer;
        frameBuffer = nullptr;
    }
    
    if (videoTexture) {
        glDeleteTextures(1, &videoTexture);
        videoTexture = 0;
    }
    
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    
    // Cleanup button resources
    if (buttonVbo) {
        glDeleteBuffers(1, &buttonVbo);
        buttonVbo = 0;
    }
    
    if (buttonVao) {
        glDeleteVertexArrays(1, &buttonVao);
        buttonVao = 0;
    }
    
    if (buttonShaderProgram) {
        glDeleteProgram(buttonShaderProgram);
        buttonShaderProgram = 0;
    }
    
    initialized = false;
}
