#include "SimpleTextRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <vector>

/*
 * Simple bitmap font implementation using colored quads.
 * Each character is rendered as a series of small rectangles in screen space.
 */

SimpleTextRenderer::SimpleTextRenderer()
    : VAO(0), VBO(0), shaderProgram(0), screenWidth(800), screenHeight(600) {
}

SimpleTextRenderer::~SimpleTextRenderer() {
    cleanup();
}

void SimpleTextRenderer::init(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    
    // Create OpenGL resources for rendering quads
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    // Reserve space for dynamic vertex data (will be updated per character)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    
    // Position attribute (2D screen space)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    
    // Texture coord attribute (for character bitmap)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
    
    compileShaders();
    
    std::cout << "[TEXT RENDERER] Initialized with screen size: " << width << "x" << height << std::endl;
}

void SimpleTextRenderer::compileShaders() {
    // Simple vertex shader for screen-space rendering
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * vec4(aPos, 0.0, 1.0);
        }
    )";
    
    // Simple fragment shader with uniform color
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        uniform vec3 textColor;
        
        void main() {
            FragColor = vec4(textColor, 1.0);
        }
    )";
    
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check vertex shader compilation
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "[TEXT RENDERER] Vertex shader compilation failed: " << infoLog << std::endl;
    }
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "[TEXT RENDERER] Fragment shader compilation failed: " << infoLog << std::endl;
    }
    
    // Link shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check linking
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "[TEXT RENDERER] Shader program linking failed: " << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    std::cout << "[TEXT RENDERER] Shaders compiled successfully" << std::endl;
}

void SimpleTextRenderer::updateScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void SimpleTextRenderer::renderText(const std::string& text, float x, float y, glm::vec3 color) {
    if (shaderProgram == 0) return;
    
    // Use text shader
    glUseProgram(shaderProgram);
    
    // Set up orthographic projection (top-left origin, y-down like typical UI)
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f);
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    
    // Set text color
    GLuint colorLoc = glGetUniformLocation(shaderProgram, "textColor");
    glUniform3f(colorLoc, color.r, color.g, color.b);
    
    glBindVertexArray(VAO);
    
    // Render each character
    float currentX = x;
    for (char c : text) {
        // Render character as a filled rectangle (simple bitmap approach)
        // For now, just render a solid quad per character
        // TODO: Implement actual bitmap font rendering with pixel patterns
        
        float charWidth = CHAR_WIDTH;
        float charHeight = CHAR_HEIGHT;
        
        // Skip spaces (don't render, just advance)
        if (c == ' ') {
            currentX += charWidth;
            continue;
        }
        
        // Create quad vertices for this character
        float vertices[6][4] = {
            { currentX,             y,              0.0f, 0.0f },
            { currentX + charWidth, y,              1.0f, 0.0f },
            { currentX + charWidth, y + charHeight, 1.0f, 1.0f },
            
            { currentX + charWidth, y + charHeight, 1.0f, 1.0f },
            { currentX,             y + charHeight, 0.0f, 1.0f },
            { currentX,             y,              0.0f, 0.0f }
        };
        
        // Update VBO with character quad
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        
        // Render character quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        // Advance to next character position
        currentX += charWidth + 2; // 2 pixels spacing between characters
    }
    
    glBindVertexArray(0);
    glUseProgram(0);
}

void SimpleTextRenderer::renderTextScaled(const std::string& text, float x, float y, glm::vec3 color, float scale) {
    if (shaderProgram == 0) return;
    
    // Use text shader
    glUseProgram(shaderProgram);
    
    // Set up orthographic projection (top-left origin, y-down like typical UI)
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f);
    GLuint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    
    // Set text color
    GLuint colorLoc = glGetUniformLocation(shaderProgram, "textColor");
    glUniform3f(colorLoc, color.r, color.g, color.b);
    
    glBindVertexArray(VAO);
    
    // Render each character with scale - pixel by pixel
    float currentX = x;
    float pixelSize = scale;  // Each "pixel" of the bitmap font is scale x scale
    float charWidth = CHAR_WIDTH * scale;
    float charHeight = CHAR_HEIGHT * scale;
    float spacing = 4.0f * scale;
    
    for (char c : text) {
        // Skip spaces (don't render, just advance)
        if (c == ' ') {
            currentX += charWidth;
            continue;
        }
        
        // Render each pixel of the character
        for (int py = 0; py < CHAR_HEIGHT; py++) {
            for (int px = 0; px < CHAR_WIDTH; px++) {
                if (getPixel(c, px, py)) {
                    float qx = currentX + px * pixelSize;
                    float qy = y + py * pixelSize;
                    
                    // Create quad vertices for this pixel
                    float vertices[6][4] = {
                        { qx,             qy,              0.0f, 0.0f },
                        { qx + pixelSize, qy,              1.0f, 0.0f },
                        { qx + pixelSize, qy + pixelSize,  1.0f, 1.0f },
                        
                        { qx + pixelSize, qy + pixelSize,  1.0f, 1.0f },
                        { qx,             qy + pixelSize,  0.0f, 1.0f },
                        { qx,             qy,              0.0f, 0.0f }
                    };
                    
                    glBindBuffer(GL_ARRAY_BUFFER, VBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
        }
        
        // Advance to next character position
        currentX += charWidth + spacing;
    }
    
    glBindVertexArray(0);
    glUseProgram(0);
}

bool SimpleTextRenderer::getPixel(char c, int x, int y) {
    // 8x12 bitmap font patterns for letters
    // Each row is 8 bits, 12 rows per character
    
    static const unsigned char font_A[12] = {
        0b00011000,  //    XX
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11111111,  // XXXXXXXX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b00000000,
    };
    
    static const unsigned char font_B[12] = {
        0b11111100,  // XXXXXX
        0b11000110,  // XX   XX
        0b11000110,  // XX   XX
        0b11000110,  // XX   XX
        0b11111100,  // XXXXXX
        0b11000110,  // XX   XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000110,  // XX   XX
        0b11111100,  // XXXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_C[12] = {
        0b00111110,  //   XXXXX
        0b01100011,  //  XX   XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b01100011,  //  XX   XX
        0b00111110,  //   XXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_D[12] = {
        0b11111100,  // XXXXXX
        0b11000110,  // XX   XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000110,  // XX   XX
        0b11111100,  // XXXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_E[12] = {
        0b11111111,  // XXXXXXXX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11111100,  // XXXXXX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11111111,  // XXXXXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_F[12] = {
        0b11111111,  // XXXXXXXX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11111100,  // XXXXXX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_G[12] = {
        0b00111110,  //   XXXXX
        0b01100011,  //  XX   XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11001111,  // XX  XXXX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100011,  //  XX   XX
        0b00111110,  //   XXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_H[12] = {
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11111111,  // XXXXXXXX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_I[12] = {
        0b01111110,  //  XXXXXX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b01111110,  //  XXXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_J[12] = {
        0b00111111,  //   XXXXXX
        0b00000110,  //      XX
        0b00000110,  //      XX
        0b00000110,  //      XX
        0b00000110,  //      XX
        0b00000110,  //      XX
        0b00000110,  //      XX
        0b11000110,  // XX   XX
        0b11000110,  // XX   XX
        0b01111100,  //  XXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_K[12] = {
        0b11000011,  // XX    XX
        0b11000110,  // XX   XX
        0b11001100,  // XX  XX
        0b11011000,  // XX XX
        0b11110000,  // XXXX
        0b11011000,  // XX XX
        0b11001100,  // XX  XX
        0b11000110,  // XX   XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_L[12] = {
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11111111,  // XXXXXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_M[12] = {
        0b11000011,  // XX    XX
        0b11100111,  // XXX  XXX
        0b11111111,  // XXXXXXXX
        0b11011011,  // XX XX XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_N[12] = {
        0b11000011,  // XX    XX
        0b11100011,  // XXX   XX
        0b11110011,  // XXXX  XX
        0b11011011,  // XX XX XX
        0b11001111,  // XX  XXXX
        0b11000111,  // XX   XXX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_O[12] = {
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_P[12] = {
        0b11111100,  // XXXXXX
        0b11000110,  // XX   XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000110,  // XX   XX
        0b11111100,  // XXXXXX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_R[12] = {
        0b11111100,  // XXXXXX
        0b11000110,  // XX   XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000110,  // XX   XX
        0b11111100,  // XXXXXX
        0b11011000,  // XX XX
        0b11001100,  // XX  XX
        0b11000110,  // XX   XX
        0b11000011,  // XX    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_S[12] = {
        0b00111110,  //   XXXXX
        0b01100011,  //  XX   XX
        0b11000000,  // XX
        0b01100000,  //  XX
        0b00111100,  //   XXXX
        0b00000110,  //      XX
        0b00000011,  //       XX
        0b00000011,  //       XX
        0b11000110,  // XX   XX
        0b01111100,  //  XXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_T[12] = {
        0b11111111,  // XXXXXXXX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_U[12] = {
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_V[12] = {
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00111100,  //   XXXX
        0b00011000,  //    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_W[12] = {
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11011011,  // XX XX XX
        0b11011011,  // XX XX XX
        0b11111111,  // XXXXXXXX
        0b11100111,  // XXX  XXX
        0b11000011,  // XX    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_X[12] = {
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_Y[12] = {
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_Z[12] = {
        0b11111111,  // XXXXXXXX
        0b00000011,  //       XX
        0b00000110,  //      XX
        0b00001100,  //     XX
        0b00011000,  //    XX
        0b00110000,  //   XX
        0b01100000,  //  XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11111111,  // XXXXXXXX
        0b00000000,
        0b00000000,
    };
    
    // Digit patterns (0-9)
    static const unsigned char font_0[12] = {
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_1[12] = {
        0b00011000,  //    XX
        0b00111000,  //   XXX
        0b01111000,  //  XXXX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b00011000,  //    XX
        0b01111110,  //  XXXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_2[12] = {
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000011,  // XX    XX
        0b00000011,  //       XX
        0b00000110,  //      XX
        0b00001100,  //     XX
        0b00011000,  //    XX
        0b00110000,  //   XX
        0b01100000,  //  XX
        0b11111111,  // XXXXXXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_3[12] = {
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000011,  // XX    XX
        0b00000011,  //       XX
        0b00011110,  //    XXXX
        0b00000011,  //       XX
        0b00000011,  //       XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_4[12] = {
        0b00000110,  //      XX
        0b00001110,  //     XXX
        0b00011110,  //    XXXX
        0b00110110,  //   XX XX
        0b01100110,  //  XX  XX
        0b11000110,  // XX   XX
        0b11111111,  // XXXXXXXX
        0b00000110,  //      XX
        0b00000110,  //      XX
        0b00000110,  //      XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_5[12] = {
        0b11111111,  // XXXXXXXX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11111100,  // XXXXXX
        0b00000110,  //      XX
        0b00000011,  //       XX
        0b00000011,  //       XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_6[12] = {
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000000,  // XX
        0b11000000,  // XX
        0b11111100,  // XXXXXX
        0b11000110,  // XX   XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_7[12] = {
        0b11111111,  // XXXXXXXX
        0b00000011,  //       XX
        0b00000110,  //      XX
        0b00001100,  //     XX
        0b00011000,  //    XX
        0b00110000,  //   XX
        0b00110000,  //   XX
        0b00110000,  //   XX
        0b00110000,  //   XX
        0b00110000,  //   XX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_8[12] = {
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00000000,
        0b00000000,
    };
    
    static const unsigned char font_9[12] = {
        0b00111100,  //   XXXX
        0b01100110,  //  XX  XX
        0b11000011,  // XX    XX
        0b11000011,  // XX    XX
        0b01100111,  //  XX  XXX
        0b00111111,  //   XXXXXX
        0b00000011,  //       XX
        0b00000011,  //       XX
        0b01100110,  //  XX  XX
        0b00111100,  //   XXXX
        0b00000000,
        0b00000000,
    };
    
    const unsigned char* pattern = nullptr;
    
    switch (c) {
        case 'A': case 'a': pattern = font_A; break;
        case 'B': case 'b': pattern = font_B; break;
        case 'C': case 'c': pattern = font_C; break;
        case 'D': case 'd': pattern = font_D; break;
        case 'E': case 'e': pattern = font_E; break;
        case 'F': case 'f': pattern = font_F; break;
        case 'G': case 'g': pattern = font_G; break;
        case 'H': case 'h': pattern = font_H; break;
        case 'I': case 'i': pattern = font_I; break;
        case 'J': case 'j': pattern = font_J; break;
        case 'K': case 'k': pattern = font_K; break;
        case 'L': case 'l': pattern = font_L; break;
        case 'M': case 'm': pattern = font_M; break;
        case 'N': case 'n': pattern = font_N; break;
        case 'O': case 'o': pattern = font_O; break;
        case 'P': case 'p': pattern = font_P; break;
        case 'R': case 'r': pattern = font_R; break;
        case 'S': case 's': pattern = font_S; break;
        case 'T': case 't': pattern = font_T; break;
        case 'U': case 'u': pattern = font_U; break;
        case 'V': case 'v': pattern = font_V; break;
        case 'W': case 'w': pattern = font_W; break;
        case 'X': case 'x': pattern = font_X; break;
        case 'Y': case 'y': pattern = font_Y; break;
        case 'Z': case 'z': pattern = font_Z; break;
        case '0': pattern = font_0; break;
        case '1': pattern = font_1; break;
        case '2': pattern = font_2; break;
        case '3': pattern = font_3; break;
        case '4': pattern = font_4; break;
        case '5': pattern = font_5; break;
        case '6': pattern = font_6; break;
        case '7': pattern = font_7; break;
        case '8': pattern = font_8; break;
        case '9': pattern = font_9; break;
        default: return false;
    }
    
    if (pattern && y >= 0 && y < 12 && x >= 0 && x < 8) {
        return (pattern[y] >> (7 - x)) & 1;
    }
    
    return false;
}

void SimpleTextRenderer::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    std::cout << "[TEXT RENDERER] Cleaned up" << std::endl;
}
