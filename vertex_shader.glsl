#version 330 core

in vec3 aPos;
in vec2 aTexCoord; // Add this line

out vec2 TexCoord; // Add this line

uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(aPos, 1.0f);
    TexCoord = aTexCoord; // Pass texture coordinates to fragment shader
}
