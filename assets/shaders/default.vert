#version 430 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aColor;

uniform mat4 model;

// Camera
uniform mat4 view;
uniform mat4 projection;

out vec3 Normal;
out vec2 TexCoord;
out vec3 FragPos;
out vec4 Color;

void main() {
    Normal = transpose(inverse(mat3(model))) * aNormal;
    TexCoord = aTexCoord;

    vec4 worldPos = model * vec4(aPosition, 1.0);
    FragPos = worldPos.xyz;

    Color = aColor;

    gl_Position = projection * view * worldPos;
}
