#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    // Scale down texture coords to make skybox appear more distant (zoomed out)
    TexCoords = aPos * 0.5;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Trick to make skybox always at max depth
}
