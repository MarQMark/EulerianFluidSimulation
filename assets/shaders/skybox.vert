#version 330 core

layout (location = 0) in vec3 aPosition;

out vec3 Direction;

uniform mat4 view;
uniform mat4 projection;

void main() {
    Direction = aPosition;

    mat4 viewNoTranslation = mat4(mat3(view));
    vec4 pos = projection * viewNoTranslation * vec4(aPosition, 1.0);
    gl_Position = pos.xyww;
}