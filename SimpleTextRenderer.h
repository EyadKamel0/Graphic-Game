#ifndef SIMPLE_TEXT_RENDERER_H
#define SIMPLE_TEXT_RENDERER_H

#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>

/*
 * ═══════════════════════════════════════════════════════════════════════
 *  SIMPLE TEXT RENDERER - Minimal HUD text for Starwave 3D
 * ═══════════════════════════════════════════════════════════════════════
 * 
 * Lightweight text rendering for HUD elements without external font libraries.
 * Uses a simple bitmap font rendered as colored quads in screen space.
 * 
 * Features:
 * - ASCII characters (uppercase, numbers, basic symbols)
 * - Fixed-width font for easy alignment
 * - Color support via RGB parameters
 * - Screen-space rendering (pixels, not world units)
 * 
 * Usage:
 *   SimpleTextRenderer textRenderer;
 *   textRenderer.init(windowWidth, windowHeight);
 *   textRenderer.renderText("ENERGY: 3/5", 10, 10, glm::vec3(1.0f, 1.0f, 0.0f));
 */

class SimpleTextRenderer {
public:
    SimpleTextRenderer();
    ~SimpleTextRenderer();
    
    // Initialize text renderer with screen dimensions
    void init(int screenWidth, int screenHeight);
    
    // Render text at screen position (x, y) in pixels from top-left
    // color: RGB values (0.0 to 1.0)
    void renderText(const std::string& text, float x, float y, glm::vec3 color);
    
    // Render text with custom scale factor
    void renderTextScaled(const std::string& text, float x, float y, glm::vec3 color, float scale);
    
    // Update screen dimensions (call on window resize)
    void updateScreenSize(int screenWidth, int screenHeight);
    
    // Cleanup
    void cleanup();
    
private:
    // Screen dimensions for orthographic projection
    int screenWidth;
    int screenHeight;
    
    // OpenGL resources for text rendering
    GLuint VAO, VBO;
    GLuint shaderProgram;
    
    // Character dimensions
    static const int CHAR_WIDTH = 8;   // Width of each character in pixels
    static const int CHAR_HEIGHT = 12; // Height of each character in pixels
    
    // Compile simple shader for text rendering
    void compileShaders();
    
    // Get bitmap pattern for a character (simple 8x12 font)
    bool getPixel(char c, int x, int y);
};

#endif // SIMPLE_TEXT_RENDERER_H
