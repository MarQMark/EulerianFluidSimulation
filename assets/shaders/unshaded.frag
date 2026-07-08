#version 330 core

in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform vec3 color = vec3(1.0, 1.0, 1.0);

void main() {
    FragColor = vec4(color, 1.0f);
}
