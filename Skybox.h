#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Skybox {
public:
    Skybox();
    ~Skybox();
    
    void loadCubemap(const std::vector<std::string>& faces);
    void render(const glm::mat4& view, const glm::mat4& projection);
    
    void cleanup();
    
private:
    GLuint VAO, VBO;
    GLuint cubemapTexture;
    GLuint shaderProgram;
    
    void setupMesh();
    void setupShader();
    GLuint loadCubemapTexture(const std::vector<std::string>& faces);
};

#endif
